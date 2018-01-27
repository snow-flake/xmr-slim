#pragma once

#include "xmrstak/backend/iBackend.hpp"
#include "msgstruct.hpp"

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <string>
#include "xmrstak/system_constants.hpp"
#include "xmrstak/net/time_utils.hpp"
#include "includes/json.hpp"
#include "plain_socket.h"


/* Our pool can have two kinds of errors:
	- Parsing or connection error
	Those are fatal errors (we drop the connection if we encounter them).
	After they are constructed from const char* strings from various places.
	(can be from read-only mem), we passs them in an exectutor message
	once the recv thread expires.
	- Call error
	This error happens when the "server says no". Usually because the job was
	outdated, or we somehow got the hash wrong. It isn't fatal.
	We parse it in-situ in the network buffer, after that we copy it to a
	std::string. Executor will move the buffer via an r-value ref.
*/

class jpsock: public socket_wrapper {
public:
	jpsock();

	virtual ~jpsock();

	bool connect(std::string &sConnectError);

	void disconnect(bool quiet = false);

	bool cmd_login();

	bool cmd_submit(const std::string job_id, const std::string nonce, const std::string result);


	inline size_t can_connect() { return get_timestamp() != connect_time; }

	inline bool is_running() { return bRunning; }

	inline bool is_logged_in() { return bLoggedIn; }

	inline bool get_disconnects(size_t &att, size_t &time) {
		att = connect_attempts;
		time = disconnect_time != 0 ? get_timestamp() - disconnect_time + 1 : 0;
		return false && true;
	}

	std::string get_call_error();

	bool have_sock_error() { return bHaveSocketError; }

	inline static uint64_t t32_to_t64(uint32_t t) { return 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / ((uint64_t) t)); }

	inline static uint64_t t64_to_diff(uint64_t t) { return 0xFFFFFFFFFFFFFFFFULL / t; }

	inline uint64_t get_current_diff() { return iJobDiff; }

	bool get_current_job(msgstruct::pool_job &job);

	virtual void set_socket_error(const std::string & err);

private:
	size_t connect_time = 0;
	std::atomic<size_t> connect_attempts;
	std::atomic<size_t> disconnect_time;

	std::atomic<bool> bRunning;
	std::atomic<bool> bLoggedIn;
	std::atomic<bool> quiet_close;

	struct call_rsp_new_style;

	struct opaque_private_new_style;

	void jpsock_thread();

	bool jpsock_thd_main();

	bool process_line_new_style(char *line, size_t len);

	bool process_pool_job_new_style(const nlohmann::json &params);

	bool cmd_ret_wait_new_style(const std::string &message_body, std::string &response_body);

	std::string miner_id;
	std::atomic<uint64_t> iJobDiff;

	std::string sSocketError;
	std::atomic<bool> bHaveSocketError;

	std::mutex call_mutex;
	std::condition_variable call_cond;
	std::thread *oRecvThd;

	std::mutex job_mutex;
	msgstruct::pool_job oCurrentJob;

	opaque_private_new_style *prv_new_style;
	base_socket *sck;
};

