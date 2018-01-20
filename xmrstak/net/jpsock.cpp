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
#include <sys/socket.h> /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include "includes/StatsdClient.hpp"

#if defined(__FreeBSD__)
#include <netinet/in.h> /* Needed for IPPROTO_TCP */
#endif

inline void sock_init() {}

typedef int SOCKET;

#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

inline void sock_close(SOCKET s) {
	shutdown(s, SHUT_RDWR);
	close(s);
}

inline const char *sock_strerror(char *buf, size_t len) {
	buf[0] = '\0';

#if defined(__APPLE__) || defined(__FreeBSD__) || !defined(_GNU_SOURCE) || !defined(__GLIBC__)

	strerror_r(errno, buf, len);
	return buf;
#else
	return strerror_r(errno, buf, len);
#endif
}

inline const char *sock_gai_strerror(int err, char *buf, size_t len) {
	buf[0] = '\0';
	return gai_strerror(err);
}


class plain_socket : public base_socket {
public:
	plain_socket(jpsock *err_callback);

	bool set_hostname(const char *sAddr);

	bool connect();

	int recv(char *buf, unsigned int len);

	bool send(const char *buf);

	void close(bool free);

private:
	jpsock *pCallback;
	addrinfo *pSockAddr;
	addrinfo *pAddrRoot;
	SOCKET hSocket;
};

plain_socket::plain_socket(jpsock *err_callback) : pCallback(err_callback) {
	hSocket = INVALID_SOCKET;
	pSockAddr = nullptr;
}

bool plain_socket::set_hostname(const char *sAddr) {
	char sAddrMb[256];
	char *sTmp, *sPort;

	size_t ln = strlen(sAddr);
	if (ln >= sizeof(sAddrMb))
		return pCallback->set_socket_error("CONNECT error: Pool address overflow.");

	memcpy(sAddrMb, sAddr, ln);
	sAddrMb[ln] = '\0';

	if ((sTmp = strstr(sAddrMb, "//")) != nullptr) {
		sTmp += 2;
		memmove(sAddrMb, sTmp, strlen(sTmp) + 1);
	}

	if ((sPort = strchr(sAddrMb, ':')) == nullptr)
		return pCallback->set_socket_error("CONNECT error: Pool port number not specified, please use format <hostname>:<port>.");

	sPort[0] = '\0';
	sPort++;

	addrinfo hints = {0};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	pAddrRoot = nullptr;
	int err;
	if ((err = getaddrinfo(sAddrMb, sPort, &hints, &pAddrRoot)) != 0)
		return pCallback->set_socket_error_strerr("CONNECT error: GetAddrInfo: ", err);

	addrinfo *ptr = pAddrRoot;
	std::vector<addrinfo *> ipv4;
	std::vector<addrinfo *> ipv6;

	while (ptr != nullptr) {
		if (ptr->ai_family == AF_INET) {
			ipv4.push_back(ptr);
		}
		if (ptr->ai_family == AF_INET6) {
			ipv6.push_back(ptr);
		}
		ptr = ptr->ai_next;
	}

	if (ipv4.empty() && ipv6.empty()) {
		freeaddrinfo(pAddrRoot);
		pAddrRoot = nullptr;
		return pCallback->set_socket_error("CONNECT error: I found some DNS records but no IPv4 or IPv6 addresses.");
	} else if (!ipv4.empty() && ipv6.empty()) {
		pSockAddr = ipv4[rand() % ipv4.size()];
	} else if (ipv4.empty() && !ipv6.empty()) {
		pSockAddr = ipv6[rand() % ipv6.size()];
	} else if (!ipv4.empty() && !ipv6.empty()) {
		pSockAddr = ipv4[rand() % ipv4.size()];
	}

	hSocket = socket(pSockAddr->ai_family, pSockAddr->ai_socktype, pSockAddr->ai_protocol);
	if (hSocket == INVALID_SOCKET) {
		freeaddrinfo(pAddrRoot);
		pAddrRoot = nullptr;
		return pCallback->set_socket_error_strerr("CONNECT error: Socket creation failed ");
	}

	return true;
}

bool plain_socket::connect() {
	int ret = ::connect(hSocket, pSockAddr->ai_addr, (int) pSockAddr->ai_addrlen);

	freeaddrinfo(pAddrRoot);
	pAddrRoot = nullptr;

	if (ret != 0)
		return pCallback->set_socket_error_strerr("CONNECT error: ");
	else
		return true;
}

int plain_socket::recv(char *buf, unsigned int len) {
	int ret = ::recv(hSocket, buf, len, 0);

	if (ret == 0)
		pCallback->set_socket_error("RECEIVE error: socket closed");
	if (ret == SOCKET_ERROR || ret < 0)
		pCallback->set_socket_error_strerr("RECEIVE error: ");

	return ret;
}

bool plain_socket::send(const char *buf) {
	int pos = 0, slen = strlen(buf);
	while (pos != slen) {
		int ret = ::send(hSocket, buf + pos, slen - pos, 0);
		if (ret == SOCKET_ERROR) {
			pCallback->set_socket_error_strerr("SEND error: ");
			return false;
		} else
			pos += ret;
	}

	return true;
}

void plain_socket::close(bool free) {
	if (hSocket != INVALID_SOCKET) {
		sock_close(hSocket);
		hSocket = INVALID_SOCKET;
	}
}


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
	sock_init();

	bJsonCallMem = (uint8_t *) malloc(iJsonMemSize);
	bJsonRecvMem = (uint8_t *) malloc(iJsonMemSize);
	bJsonParseMem = (uint8_t *) malloc(iJsonMemSize);

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

	free(bJsonCallMem);
	free(bJsonRecvMem);
	free(bJsonParseMem);
}

std::string jpsock::get_call_error() {
	if (prv_new_style->oCallRsp.get() != nullptr) {
		return prv_new_style->oCallRsp->sCallErr;
	}
	return "";
}

bool jpsock::set_socket_error(const char *a) {
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };
	client.increment("socket_error");

	if (!bHaveSocketError) {
		bHaveSocketError = true;
		sSocketError.assign(a);
	}

	return false;
}

bool jpsock::set_socket_error(const char *a, const char *b) {
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };
	client.increment("socket_error");

	if (!bHaveSocketError) {
		bHaveSocketError = true;
		size_t ln_a = strlen(a);
		size_t ln_b = strlen(b);

		sSocketError.reserve(ln_a + ln_b + 2);
		sSocketError.assign(a, ln_a);
		sSocketError.append(b, ln_b);
	}

	return false;
}


bool jpsock::set_socket_error_strerr(const char *a) {
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };
	client.increment("socket_error");

	char sSockErrText[512];
	return set_socket_error(a, sock_strerror(sSockErrText, sizeof(sSockErrText)));
}

bool jpsock::set_socket_error_strerr(const char *a, int res) {
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix.", 20 };
	client.increment("socket_error");

	char sSockErrText[512];
	return set_socket_error(a, sock_gai_strerror(res, sSockErrText, sizeof(sSockErrText)));
}

void jpsock::jpsock_thread() {
	jpsock_thd_main();
	executor::inst()->push_event(ex_event(std::move(sSocketError), quiet_close));

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

	executor::inst()->push_event(ex_event(EV_SOCK_READY));

	char buf[iSockBufferSize];
	size_t datalen = 0;
	while (true) {
		int ret = sck->recv(buf + datalen, sizeof(buf) - datalen);

		if (ret <= 0)
			return false;

		datalen += ret;

		if (datalen >= sizeof(buf)) {
			sck->close(false);
			return set_socket_error("RECEIVE error: data overflow");
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
		return set_socket_error("PARSE error: Invalid root");
	}

	if (data.find("method") != data.end()) {
		if (data["method"].is_null() || !data["method"].is_string()) {
			return set_socket_error("PARSE error: Protocol error 1");
		}

		const std::string method = data["method"].get<std::string>();
		if (method != "job") {
			return set_socket_error("PARSE error: Unsupported server method ", method.c_str());
		}

		if (data["params"].is_null() || !data["params"].is_object()) {
			return set_socket_error("PARSE error: Protocol error 2");
		}

		auto params = data["params"];
		return process_pool_job_new_style(params);
	} else {
		if (data["id"].is_null() || !data["id"].is_number()) {
			return set_socket_error("PARSE error: Protocol error 3");
		}

		const uint64_t iCallId = data["id"].get<uint64_t>();

		auto error_iter = data.find("error");
		auto result_iter = data.find("result");

		std::string sError = "N/A";

		if (error_iter == data.end()) {
			/* If there was no error we need a result */
			if (result_iter == data.end() || result_iter->is_null()) {
				return set_socket_error("PARSE error: Protocol error 7");
			}
		} else if (error_iter->is_null()) {
			/* If there was no error we need a result */
			if (result_iter == data.end() || result_iter->is_null()) {
				return set_socket_error("PARSE error: Protocol error 7");
			}
		} else if (!error_iter->is_object()) {
			return set_socket_error("PARSE error: Protocol error 5");
		} else {
			auto error = data["error"];
			auto message = error["message"];
			if (message.is_null() || !message.is_string()) {
				return set_socket_error("PARSE error: Protocol error 6");
			}
			sError = message.get<std::string>();
		}

		std::unique_lock<std::mutex> mlock(call_mutex);
		if (prv_new_style->oCallRsp.get() == nullptr) {
			/*Server sent us a call reply without us making a call*/
			mlock.unlock();
			return set_socket_error("PARSE error: Unexpected call response");
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
		return set_socket_error("PARSE error: Job error 1");
	}

	//	const Value *blob, *jobid, *target, *motd;
	//	const auto job_id = params["job_id"];  // jobid = GetObjectMember(*params->val, "job_id");
	//	const auto blob = params["blob"];  // blob = GetObjectMember(*params->val, "blob");
	//	const auto target = params["target"];  // target = GetObjectMember(*params->val, "target");
	//	const auto motd = params["motd"];  // motd = GetObjectMember(*params->val, "motd");

	if (params["job_id"].is_null() || !params["job_id"].is_string()) {
		return set_socket_error("PARSE error: Job error 2");
	}
	if (params["blob"].is_null() || !params["blob"].is_string()) {
		return set_socket_error("PARSE error: Job error 2");
	}
	if (params["target"].is_null() || !params["target"].is_string()) {
		return set_socket_error("PARSE error: Job error 2");
	}

	const std::string job_id = params["job_id"].get<std::string>();
	const std::string blob = params["blob"].get<std::string>();
	const std::string target = params["target"].get<std::string>();

	// Note >=
	if (job_id.length() >= sizeof(pool_job::sJobID)) {
		return set_socket_error("PARSE error: Job error 3");
	}

	//	uint32_t iWorkLn = blob->GetStringLength() / 2;
	if (blob.length() / 2 > sizeof(pool_job::bWorkBlob)) {
		return set_socket_error("PARSE error: Invalid job legth. Are you sure you are mining the correct coin?");
	}

	pool_job oPoolJob;
	if (!hex2bin(blob.c_str(), blob.length(), oPoolJob.bWorkBlob)) {
		return set_socket_error("PARSE error: Job error 4");
	}

	oPoolJob.iWorkLen = blob.length() / 2;
	memset(oPoolJob.sJobID, 0, sizeof(pool_job::sJobID));
	strcpy(oPoolJob.sJobID, job_id.c_str());
	//	memcpy(oPoolJob.sJobID, jobid->GetString(), jobid->GetStringLength()); //Bounds checking at proto error 3

	if (target.length() <= 8) {
		uint32_t iTempInt = 0;
		char sTempStr[] = "00000000"; // Little-endian CPU FTW
		memcpy(sTempStr, target.c_str(), target.length());
		if (!hex2bin(sTempStr, 8, (unsigned char *) &iTempInt) || iTempInt == 0) {
			return set_socket_error("PARSE error: Invalid target");
		}
		oPoolJob.iTarget = t32_to_t64(iTempInt);
	} else if (target.length() <= 16) {
		oPoolJob.iTarget = 0;
		char sTempStr[] = "0000000000000000";
		memcpy(sTempStr, target.c_str(), target.length());
		if (!hex2bin(sTempStr, 16, (unsigned char *) &oPoolJob.iTarget) || oPoolJob.iTarget == 0) {
			return set_socket_error("PARSE error: Invalid target");
		}
	} else {
		return set_socket_error("PARSE error: Job error 5");
	}

	if (params.find("motd") != params.end()) {
		auto raw_motd = params["motd"];
		if (!raw_motd.is_null() && raw_motd.is_string()) {
			const std::string motd = params["motd"].get<std::string>();
			if ((motd.length() & 0x01) == 0) {
				std::unique_lock<std::mutex>(motd_mutex);
				if (motd.length() > 0) {
					pool_motd.resize(motd.length() / 2 + 1);
					if (!hex2bin(motd.c_str(), motd.length(), (unsigned char *) &pool_motd.front())) {
						pool_motd.clear();
					}
				} else {
					pool_motd.clear();
				}
			}
		}
	}


	iJobDiff = t64_to_diff(oPoolJob.iTarget);

	executor::inst()->push_event(ex_event(oPoolJob));

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
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix." };
	client.increment("login");

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

bool jpsock::cmd_submit(const char *sJobId, uint32_t iNonce, const uint8_t *bResult, xmrstak::iBackend *bend) {
	Statsd::StatsdClient client{ "127.0.0.1", 8080, "myPrefix." };
	client.increment("submit");

	char sNonce[9];
	bin2hex((unsigned char *) &iNonce, 4, sNonce);
	sNonce[8] = '\0';

	char sResult[65];
	bin2hex(bResult, 32, sResult);
	sResult[64] = '\0';

	nlohmann::json data;
	data["method"] = "submit";
	data["params"]["id"] = sMinerId;
	data["params"]["job_id"] = sJobId;
	data["params"]["nonce"] = sNonce;
	data["params"]["result"] = sResult;
	data["id"] = 1;

	const std::string cmd_buffer = data.dump();
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_submit: " << cmd_buffer << std::endl;

	std::string response_body;
	const bool success = cmd_ret_wait_new_style(cmd_buffer, response_body);
	std::cout << __FILE__ << ":" << __LINE__ << ":jpsock::cmd_submit: response=" << response_body << std::endl;
	return success;
}

bool jpsock::get_current_job(pool_job &job) {
	std::unique_lock<std::mutex>(job_mutex);

	if (oCurrentJob.iWorkLen == 0)
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

inline unsigned char hf_hex2bin(char c, bool &err) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xA;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;

	err = true;
	return 0;
}

bool jpsock::hex2bin(const char *in, unsigned int len, unsigned char *out) {
	bool error = false;
	for (unsigned int i = 0; i < len; i += 2) {
		out[i / 2] = (hf_hex2bin(in[i], error) << 4) | hf_hex2bin(in[i + 1], error);
		if (error) return false;
	}
	return true;
}

inline char hf_bin2hex(unsigned char c) {
	if (c <= 0x9)
		return '0' + c;
	else
		return 'a' - 0xA + c;
}

void jpsock::bin2hex(const unsigned char *in, unsigned int len, char *out) {
	for (unsigned int i = 0; i < len; i++) {
		out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
		out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
	}
}
