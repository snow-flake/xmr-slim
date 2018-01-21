#ifndef XMR_SLIM_JPSOC_V2_HPP
#define XMR_SLIM_JPSOC_V2_HPP

#include "xmrstak/backend/iBackend.hpp"
#include "msgstruct_v2.hpp"

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

namespace jpsock_v2_grp {

	struct call_rsp_new_style {
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


	class jpsock_v2 : public socket_wrapper {
	public:
		jpsock_v2();

		virtual ~jpsock_v2();

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

		bool get_pool_motd(std::string &strin);

		std::string get_call_error();

		bool have_sock_error() { return bHaveSocketError; }

		inline static uint64_t t32_to_t64(uint32_t t) { return 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / ((uint64_t) t)); }

		inline static uint64_t t64_to_diff(uint64_t t) { return 0xFFFFFFFFFFFFFFFFULL / t; }

		inline uint64_t get_current_diff() { return iJobDiff; }

		std::shared_ptr<const msgstruct_v2::pool_job> get_current_job();

		virtual void set_socket_error(const std::string &err);

	private:
		bool ext_motd = false;

		size_t connect_time = 0;
		std::atomic<size_t> connect_attempts;
		std::atomic<size_t> disconnect_time;

		std::atomic<bool> bRunning;
		std::atomic<bool> bLoggedIn;
		std::atomic<bool> quiet_close;

		std::shared_ptr<call_rsp_new_style> oCallRsp;
		std::shared_ptr<std::string> oCallReq;

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
		std::shared_ptr<const msgstruct_v2::pool_job> oCurrentJob;

		base_socket *sck;
	};

}
#endif //XMR_SLIM_JPSOC_V2_HPP