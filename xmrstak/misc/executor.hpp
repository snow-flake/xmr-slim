#pragma once

#include "thdq.hpp"
#include "telemetry.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/misc/environment.hpp"
#include "xmrstak/net/msgstruct.hpp"
#include "xmrstak/net/time_utils.hpp"
#include <memory>
#include <array>
#include <list>
#include <future>

class jpsock;

namespace xmrstak
{
namespace cpu
{
class minethd;

} // namespace cpu
} // namepsace xmrstak

class executor
{
public:
	static executor* inst()
	{
		auto& env = xmrstak::environment::inst();
		if(env.pExecutor == nullptr)
			env.pExecutor = new executor;
		return env.pExecutor;
	};

	void ex_main();

	inline void push_event(const msgstruct::ex_event_const_ptr_t & ev) { event_queue.push(ev); }
	void push_timed_event(const msgstruct::ex_event_const_ptr_t & ev, size_t sec);

private:
	struct timed_event
	{
		msgstruct::ex_event_const_ptr_t event;
		size_t ticks_left;
		timed_event(const msgstruct::ex_event_const_ptr_t & ev, size_t ticks) : event(ev), ticks_left(ticks) {}
	};

	inline void set_timestamp() { dev_timestamp = get_timestamp(); };

	// In miliseconds, has to divide a second (1000ms) into an integer number
	constexpr static size_t iTickTime = 500;

	std::list<timed_event> lTimedEvents;
	std::mutex timed_event_mutex;
	thdq<msgstruct::ex_event_const_ptr_t> event_queue;

	xmrstak::telemetry* telem;
	std::vector<xmrstak::iBackend*>* pvThreads;

	size_t dev_timestamp;

	std::shared_ptr<jpsock> pool_ptr;

	executor();


	void ex_clock_thd();

	constexpr static size_t motd_max_length = 512;
	bool motd_filter_console(std::string& motd);
	std::string hashrate_report();
	std::string result_report();
	std::string connection_report();
	void print_report();

	struct sck_error_log
	{
		std::chrono::system_clock::time_point time;
		std::string msg;

		sck_error_log(std::string&& err) : msg(std::move(err))
		{
			time = std::chrono::system_clock::now();
		}
	};
	std::vector<sck_error_log> vSocketLog;

	// Element zero is always the success element.
	// Keep in mind that this is a tally and not a log like above
	struct result_tally
	{
		std::chrono::system_clock::time_point time;
		std::string msg;
		size_t count;

		result_tally() : msg("[OK]"), count(0)
		{
			time = std::chrono::system_clock::now();
		}

		result_tally(std::string&& err) : msg(std::move(err)), count(1)
		{
			time = std::chrono::system_clock::now();
		}

		void increment()
		{
			count++;
			time = std::chrono::system_clock::now();
		}

		bool compare(std::string& err)
		{
			if(msg == err)
				return true;
			else
				return false;
		}
	};
	std::vector<result_tally> vMineResults;

	//More result statistics
	std::array<size_t, 10> iTopDiff { { } }; //Initialize to zero

	std::chrono::system_clock::time_point tPoolConnTime;
	size_t iPoolHashes = 0;
	uint64_t iPoolDiff = 0;

	// Set it to 16 bit so that we can just let it grow
	// Maximum realistic growth rate - 5MB / month
	std::vector<uint16_t> iPoolCallTimes;

	//Those stats are reset if we disconnect
	inline void reset_stats()
	{
		iPoolCallTimes.clear();
		tPoolConnTime = std::chrono::system_clock::now();
		iPoolHashes = 0;
		iPoolDiff = 0;
	}

	double fHighestHps = 0.0;

	void log_socket_error(std::string&& sError);
	void log_result_error(std::string&& sError);
	void log_result_ok(uint64_t iActualDiff);

	void on_sock_ready();

	void on_sock_error(std::string&& sError, bool silent);
	void on_pool_have_job(const msgstruct::pool_job_const_ptr_t oPoolJob);
	void on_miner_result(const msgstruct::job_result_const_ptr_t oResult);

	bool is_pool_live();
	void eval_pool_choice();

	inline size_t sec_to_ticks(size_t sec) { return sec * (1000 / iTickTime); }
};

