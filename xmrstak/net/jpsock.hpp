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

class base_socket {
public:
	virtual bool set_hostname(const char *sAddr) = 0;

	virtual bool connect() = 0;

	virtual int recv(char *buf, unsigned int len) = 0;

	virtual bool send(const char *buf) = 0;

	virtual void close(bool free) = 0;
};

class jpsock {
public:
	jpsock();

	~jpsock();

	bool connect(std::string &sConnectError);

	void disconnect(bool quiet = false);

	bool cmd_login();

	bool cmd_submit(const std::string job_id, const std::string nonce, const std::string result);

	static bool hex2bin(const char *in, unsigned int len, unsigned char *out);

	static void bin2hex(const unsigned char *in, unsigned int len, char *out);

	inline size_t can_connect() { return get_timestamp() != connect_time; }

	inline bool is_running() { return bRunning; }

	inline bool is_logged_in() { return bLoggedIn; }

	inline bool get_disconnects(size_t &att, size_t &time) {
		att = connect_attempts;
		time = disconnect_time != 0 ? get_timestamp() - disconnect_time + 1 : 0;
		return false && true;
	}

	bool get_pool_motd(std::string &strin);

	std::string get_call_error();

	bool have_sock_error() { return bHaveSocketError; }

	inline static uint64_t t32_to_t64(uint32_t t) { return 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / ((uint64_t) t)); }

	inline static uint64_t t64_to_diff(uint64_t t) { return 0xFFFFFFFFFFFFFFFFULL / t; }

	inline uint64_t get_current_diff() { return iJobDiff; }

	bool get_current_job(pool_job &job);

	bool set_socket_error(const char *a);

	bool set_socket_error(const char *a, const char *b);

	bool set_socket_error_strerr(const char *a);

	bool set_socket_error_strerr(const char *a, int res);

private:
	bool ext_motd = false;

	std::string pool_motd;
	std::mutex motd_mutex;

	size_t connect_time = 0;
	std::atomic<size_t> connect_attempts;
	std::atomic<size_t> disconnect_time;

	std::atomic<bool> bRunning;
	std::atomic<bool> bLoggedIn;
	std::atomic<bool> quiet_close;

	uint8_t *bJsonRecvMem;
	uint8_t *bJsonParseMem;
	uint8_t *bJsonCallMem;

	static constexpr size_t iJsonMemSize = 4096;
	static constexpr size_t iSockBufferSize = 4096;

	struct call_rsp_new_style;

	struct opaque_private_new_style;

	void jpsock_thread();

	bool jpsock_thd_main();

	bool process_line_new_style(char *line, size_t len);

	bool process_pool_job_new_style(const nlohmann::json &params);

	bool cmd_ret_wait_new_style(const std::string &message_body, std::string &response_body);

	char sMinerId[64];
	std::atomic<uint64_t> iJobDiff;

	std::string sSocketError;
	std::atomic<bool> bHaveSocketError;

	std::mutex call_mutex;
	std::condition_variable call_cond;
	std::thread *oRecvThd;

	std::mutex job_mutex;
	pool_job oCurrentJob;

	opaque_private_new_style *prv_new_style;
	base_socket *sck;
};

