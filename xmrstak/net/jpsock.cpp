/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include <assert.h>
#include <algorithm>
#include "jpsock.hpp"
#include "xmrstak/misc/executor.hpp"
#include "xmrstak/cli/statsd.hpp"


struct jpsock::call_rsp_new_style {
	const bool bHaveResponse;
	const bool bHaveError;
	const uint64_t iCallId;
	const std::string pCallData;
	const std::string sCallErr;

	call_rsp_new_style() : iCallId(0), pCallData(), sCallErr(), bHaveResponse(false), bHaveError(false) {
	}

	call_rsp_new_style(uint64_t iCallId, const nlohmann::json &data) : iCallId(iCallId), pCallData(data.dump()), sCallErr(), bHaveResponse(true), bHaveError(false) {
	}

	call_rsp_new_style(uint64_t iCallId, const std::string &error) : iCallId(iCallId), pCallData(), sCallErr(error), bHaveResponse(false), bHaveError(true) {
	}
};


/*
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * ASSUMPTION - only one calling thread. Multiple calling threads would require better
 * thread safety. The calling thread is assumed to be the executor thread.
 * If there is a reason to call the pool outside of the executor context, consider
 * doing it via an executor event.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Call values and allocators are for the calling thread (executor). When processing
 * a call, the recv thread will make a copy of the call response and then erase its copy.
 */

struct jpsock::opaque_private_new_style {
	std::shared_ptr<jpsock::call_rsp_new_style> oCallRsp;
	std::shared_ptr<std::string> oCallReq;

	opaque_private_new_style() {
	}
};

jpsock::jpsock() : connect_time(0), connect_attempts(0), disconnect_time(0), quiet_close(false) {
	prv_new_style = new opaque_private_new_style();
	sck = new plain_socket(this);

	oRecvThd = nullptr;
	bRunning = false;
	bLoggedIn = false;
	iJobDiff = 0;

	memset(&oCurrentJob, 0, sizeof(oCurrentJob));
}

jpsock::~jpsock() {
	delete prv_new_style;
	prv_new_style = nullptr;
}

std::string jpsock::get_call_error() {
	if (prv_new_style->oCallRsp.get() != nullptr) {
		return prv_new_style->oCallRsp->sCallErr;
	}
	return "";
}

void jpsock::set_socket_error(const std::string & err) {
	statsd::statsd_increment("socket_error");
	if (!bHaveSocketError) {
		bHaveSocketError = true;
		sSocketError = err;
	}
}




void jpsock::jpsock_thread() {
	jpsock_thd_main();

	std::string err(std::move(sSocketError));
	executor::inst()->push_event_error(err, quiet_close);

	// If a call is wating, send an error to end it
	bool bCallWaiting = false;
	std::unique_lock<std::mutex> mlock(call_mutex);
	if (prv_new_style->oCallReq.get() != nullptr) {
		prv_new_style->oCallRsp = std::shared_ptr<jpsock::call_rsp_new_style>(
				new jpsock::call_rsp_new_style(
						0,
						std::string("")
				)
		);
		bCallWaiting = true;
	}

	mlock.unlock();

	if (bCallWaiting)
		call_cond.notify_one();

	bLoggedIn = false;

	if (bHaveSocketError && !quiet_close)
		disconnect_time = get_timestamp();
	else
		disconnect_time = 0;

	std::unique_lock<std::mutex>(job_mutex);
	memset(&oCurrentJob, 0, sizeof(oCurrentJob));
	bRunning = false;
}

bool jpsock::jpsock_thd_main() {
	if (!sck->connect())
		return false;

	executor::inst()->push_event_name(msgstruct::EV_SOCK_READY);

	static constexpr size_t iSockBufferSize = 4096;
	char buf[iSockBufferSize];
	size_t datalen = 0;
	while (true) {
		int ret = sck->recv(buf + datalen, sizeof(buf) - datalen);

		if (ret <= 0)
			return false;

		datalen += ret;

		if (datalen >= sizeof(buf)) {
			sck->close(false);
			set_socket_error("RECEIVE error: data overflow");
			return false;
		}

		char *lnend;
		char *lnstart = buf;
		while ((lnend = (char *) memchr(lnstart, '\n', datalen)) != nullptr) {
			lnend++;
			int lnlen = lnend - lnstart;

			if (!process_line_new_style(lnstart, lnlen)) {
				sck->close(false);
				return false;
			}

			datalen -= lnlen;
			lnstart = lnend;
		}

		//Got leftover data? Move it to the front
		if (datalen > 0 && buf != lnstart)
			memmove(buf, lnstart, datalen);
	}
}

bool jpsock::process_line_new_style(char *line, size_t len) {
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::process_line_new_style: " << line << std::endl;

	/*NULL terminate the line instead of '\n', parsing will add some more NULLs*/
	line[len - 1] = '\0';

	auto data = nlohmann::json::parse(line);
	if (!data.is_object()) {
		set_socket_error("PARSE error: Invalid root");
		return false;
	}

	if (data.find("method") != data.end()) {
		if (data["method"].is_null() || !data["method"].is_string()) {
			set_socket_error("PARSE error: Protocol error 1");
			return false;
		}

		const std::string method = data["method"].get<std::string>();
		if (method != "job") {
			set_socket_error("PARSE error: Unsupported server method " + method);
			return false;
		}

		if (data["params"].is_null() || !data["params"].is_object()) {
			set_socket_error("PARSE error: Protocol error 2");
			return false;
		}

		auto params = data["params"];
		return process_pool_job_new_style(params);
	} else {
		if (data["id"].is_null() || !data["id"].is_number()) {
			set_socket_error("PARSE error: Protocol error 3");
			return false;
		}

		const uint64_t iCallId = data["id"].get<uint64_t>();

		auto error_iter = data.find("error");
		auto result_iter = data.find("result");

		std::string sError = "N/A";

		if (error_iter == data.end()) {
			/* If there was no error we need a result */
			if (result_iter == data.end() || result_iter->is_null()) {
				set_socket_error("PARSE error: Protocol error 7");
				return false;
			}
		} else if (error_iter->is_null()) {
			/* If there was no error we need a result */
			if (result_iter == data.end() || result_iter->is_null()) {
				set_socket_error("PARSE error: Protocol error 7");
				return false;
			}
		} else if (!error_iter->is_object()) {
			set_socket_error("PARSE error: Protocol error 5");
			return false;
		} else {
			auto error = data["error"];
			auto message = error["message"];
			if (message.is_null() || !message.is_string()) {
				set_socket_error("PARSE error: Protocol error 6");
				return false;
			}
			sError = message.get<std::string>();
		}

		std::unique_lock<std::mutex> mlock(call_mutex);
		if (prv_new_style->oCallRsp.get() == nullptr) {
			/*Server sent us a call reply without us making a call*/
			mlock.unlock();
			set_socket_error("PARSE error: Unexpected call response");
			return false;
		}

		if (sError != "N/A") {
			prv_new_style->oCallRsp = std::shared_ptr<jpsock::call_rsp_new_style>(
					new jpsock::call_rsp_new_style(
							iCallId,
							sError
					)
			);
		} else {
			prv_new_style->oCallRsp = std::shared_ptr<jpsock::call_rsp_new_style>(
					new jpsock::call_rsp_new_style(
							iCallId,
							*result_iter
					)
			);
		}

		mlock.unlock();
		call_cond.notify_one();

		return true;
	}
}

bool jpsock::process_pool_job_new_style(const nlohmann::json &params) {
	if (!params.is_object()) {
		set_socket_error("PARSE error: Job error 1");
		return false;
	}
	if (params["job_id"].is_null() || !params["job_id"].is_string()) {
		set_socket_error("PARSE error: Job error 2");
		return false;
	}
	if (params["blob"].is_null() || !params["blob"].is_string()) {
		set_socket_error("PARSE error: Job error 2");
		return false;
	}
	if (params["target"].is_null() || !params["target"].is_string()) {
		set_socket_error("PARSE error: Job error 2");
		return false;
	}

	const std::string job_id = params["job_id"].get<std::string>();
	const std::string blob = params["blob"].get<std::string>();
	const std::string target = params["target"].get<std::string>();

	// Note >=
	if (job_id.length() >= sizeof(msgstruct::pool_job::job_id_data)) {
		set_socket_error("PARSE error: Job error 3");
		return false;
	}

	if (blob.length() / 2 > sizeof(msgstruct::pool_job::work_blob_data)) {
		set_socket_error("PARSE error: Invalid job legth. Are you sure you are mining the correct coin?");
		return false;
	}

	msgstruct::pool_job oPoolJob;
	oPoolJob.target = target;
	iJobDiff = oPoolJob.i_job_diff();

	if (!msgstruct_v2::utils::hex2bin(blob.c_str(), blob.length(), &oPoolJob.work_blob_data[0])) {
		set_socket_error("PARSE error: Job error 4");
		return false;
	}

	oPoolJob.work_blob_len = blob.length() / 2;
	oPoolJob.job_id_data.fill(0);
	strcpy(&oPoolJob.job_id_data[0], job_id.c_str());


	if (params.find("motd") != params.end()) {
		auto raw_motd = params["motd"];
		if (!raw_motd.is_null() && raw_motd.is_string()) {
			const std::string motd = params["motd"].get<std::string>();
			if ((motd.length() & 0x01) == 0) {
				std::unique_lock<std::mutex>(motd_mutex);
				if (motd.length() > 0) {
					pool_motd.resize(motd.length() / 2 + 1);
					if (!msgstruct_v2::utils::hex2bin(motd.c_str(), motd.length(), (unsigned char *) &pool_motd.front())) {
						pool_motd.clear();
					}
				} else {
					pool_motd.clear();
				}
			}
		}
	}

	executor::inst()->push_event_pool_job(oPoolJob);

	std::unique_lock<std::mutex>(job_mutex);
	oCurrentJob = oPoolJob;
	return true;
}

bool jpsock::connect(std::string &sConnectError) {
	ext_motd = false;
	bHaveSocketError = false;
	sSocketError.clear();
	iJobDiff = 0;
	connect_attempts++;
	connect_time = get_timestamp();

	const std::string host_name = system_constants::get_pool_pool_address();
	if (sck->set_hostname(host_name.c_str())) {
		bRunning = true;
		disconnect_time = 0;
		oRecvThd = new std::thread(&jpsock::jpsock_thread, this);
		return true;
	}

	disconnect_time = get_timestamp();
	sConnectError = std::move(sSocketError);
	return false;
}

void jpsock::disconnect(bool quiet) {
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::disconnect: Disconnecting from pool" << std::endl;
	quiet_close = quiet;
	sck->close(false);

	if (oRecvThd != nullptr) {
		oRecvThd->join();
		delete oRecvThd;
		oRecvThd = nullptr;
	}

	sck->close(true);
	quiet_close = false;
}

bool jpsock::cmd_ret_wait_new_style(const std::string &message_body, std::string &response_body) {
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_ret_wait: " << message_body << std::endl;

	/*Set up the call rsp for the call reply*/
	prv_new_style->oCallRsp = std::shared_ptr<jpsock::call_rsp_new_style>(new jpsock::call_rsp_new_style());

	std::unique_lock<std::mutex> mlock(call_mutex);
	mlock.unlock();

	if (!sck->send(message_body.c_str())) {
		std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_ret_wait:" << "Failed, disconnecting" << std::endl;
		disconnect(); //This will join the other thread;
		return false;
	}

	//Success is true if the server approves, result is true if there was no socket error
	mlock.lock();
	bool bResult = call_cond.wait_for(
			mlock,
			std::chrono::seconds(system_constants::GetCallTimeout()),
			[&]() { return prv_new_style->oCallRsp.get() != nullptr && prv_new_style->oCallRsp->bHaveResponse; }
	);

	mlock.unlock();

	// TODO: who sets this?
	if (bHaveSocketError) {
		return false;
	}

	//This means that there was no socket error, but the server is not taking to us
	if (!prv_new_style->oCallRsp->bHaveResponse) {
		set_socket_error("CALL error: Timeout while waiting for a reply");
		disconnect();
		return false;
	}

	if (prv_new_style->oCallRsp->bHaveResponse) {
		response_body = prv_new_style->oCallRsp->pCallData;
	}

	return prv_new_style->oCallRsp->bHaveResponse;
}

bool jpsock::cmd_login() {
	statsd::statsd_increment("login");

	nlohmann::json data;
	data["method"] = "login";
	data["id"] = 1;
	data["params"]["login"] = system_constants::get_pool_wallet_address();
	data["params"]["pass"] = system_constants::get_pool_pool_password();
	data["params"]["agent"] = system_constants::get_version_str();

	std::string cmd_buffer = data.dump();

	/*Normal error conditions (failed login etc..) will end here*/
	std::string response_body;
	if (!cmd_ret_wait_new_style(cmd_buffer, response_body)) {
		return false;
	}

	data = nlohmann::json::parse(response_body);
	if (!data.is_object()) {
		set_socket_error("PARSE error: Login protocol error 1");
		disconnect();
		return false;
	}

	if (data["id"].is_null() || !data["id"].is_string() || data["job"].is_null()) {
		set_socket_error("PARSE error: Login protocol error 2");
		disconnect();
		return false;
	}

	const std::string id = data["id"].get<std::string>();
	if (id.length() >= sizeof(sMinerId)) {
		set_socket_error("PARSE error: Login protocol error 3");
		disconnect();
		return false;
	}

	memset(sMinerId, 0, sizeof(sMinerId));
	strcpy(sMinerId, id.c_str());

	if (!data["extensions"].is_null() && data["extensions"].is_array()) {
		for (auto &element: data["extensions"]) {
			if (!element.is_string()) {
				continue;
			}

			std::string tmp = element.get<std::string>();
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
			if (tmp == "motd")
				ext_motd = true;
		}
	}

	auto params = data["job"];
	if (!process_pool_job_new_style(params)) {
		disconnect();
		return false;
	}

	bLoggedIn = true;
	connect_attempts = 0;
	return true;
}

bool jpsock::cmd_submit(const std::string job_id, std::string nonce, const std::string result) {
	statsd::statsd_increment("submit");

	nlohmann::json data;
	data["method"] = "submit";
	data["params"]["id"] = sMinerId;
	data["params"]["job_id"] = job_id;
	data["params"]["nonce"] = nonce;
	data["params"]["result"] = result;
	data["id"] = 1;

	const std::string cmd_buffer = data.dump();
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_submit: " << cmd_buffer << std::endl;

	std::string response_body;
	const bool success = cmd_ret_wait_new_style(cmd_buffer, response_body);
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_submit: response=" << response_body << std::endl;
	return success;
}

bool jpsock::get_current_job(msgstruct::pool_job &job) {
	std::unique_lock<std::mutex>(job_mutex);

	if (oCurrentJob.work_blob_len == 0)
		return false;

	job = oCurrentJob;
	return true;
}

bool jpsock::get_pool_motd(std::string &strin) {
	if (!ext_motd)
		return false;

	std::unique_lock<std::mutex>(motd_mutex);
	if (pool_motd.size() > 0) {
		strin.assign(pool_motd);
		return true;
	}

	return false;
}
