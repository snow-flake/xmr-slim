#pragma once

#include "thdq.hpp"
#include "telemetry.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/misc/environment.hpp"
#include "xmrstak/net/msgstruct.hpp"
#include "xmrstak/net/time_utils.hpp"
#include "xmrstak/net/jpsock.hpp"
#include <memory>
#include <array>
#include <list>
#include <future>
#include <memory>


struct timed_event {
	std::shared_ptr<const msgstruct::ex_event> event_ptr;
	size_t ticks_left;
	timed_event(std::shared_ptr<const msgstruct::ex_event> ev, size_t ticks) : event_ptr(ev), ticks_left(ticks) {}
};


// Element zero is always the success element.
// Keep in mind that this is a tally and not a log like above
struct result_tally {
	std::chrono::system_clock::time_point time;
	std::string msg;
	size_t count;

	result_tally() : msg("[OK]"), count(0) {
		time = std::chrono::system_clock::now();
	}

	result_tally(std::string&& err) : count(1) {
		time = std::chrono::system_clock::now();
		msg = err;
		err = "";
	}

	void increment() {
		count++;
		time = std::chrono::system_clock::now();
	}

	bool compare(std::string& err) {
		return msg == err;
	}
};


struct sck_error_log {
	std::chrono::system_clock::time_point time;
	std::string msg;

	sck_error_log(std::string&& err) {
		time = std::chrono::system_clock::now();
		msg = err;
		err = "";
	}
};


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

	inline void push_event(const std::shared_ptr<const msgstruct::ex_event> & ev) { oEventQ.push(ev); }

	inline void push_event_name(const msgstruct::ex_event_name name) {
		std::shared_ptr<const msgstruct::ex_event> ptr = std::shared_ptr<const msgstruct::ex_event>(
				new msgstruct::ex_event(name)
		);
		oEventQ.push(ptr);
	}

	inline void push_event_error(const std::string & error, bool silent) {
		std::shared_ptr<const msgstruct::ex_event> ptr = std::shared_ptr<const msgstruct::ex_event>(
				new msgstruct::ex_event(error, silent)
		);
		oEventQ.push(ptr);
	}

	inline void push_event_job_result(const msgstruct::job_result & result) {
		std::shared_ptr<const msgstruct::ex_event> ptr = std::shared_ptr<const msgstruct::ex_event>(
				new msgstruct::ex_event(result)
		);
		oEventQ.push(ptr);
	}


	inline void push_event_pool_job(const msgstruct::pool_job & job) {
		std::shared_ptr<const msgstruct::ex_event> ptr = std::shared_ptr<const msgstruct::ex_event>(
				new msgstruct::ex_event(job)
		);
		oEventQ.push(ptr);
	}

	void push_timed_event(msgstruct::ex_event_name name, size_t sec);

private:

	inline void set_timestamp() { dev_timestamp = get_timestamp(); };

	// In miliseconds, has to divide a second (1000ms) into an integer number
	constexpr static size_t iTickTime = 500;

	std::list<timed_event> lTimedEvents;
	std::mutex timed_event_mutex;
	thdq<std::shared_ptr<const msgstruct::ex_event>> oEventQ;

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

	std::vector<sck_error_log> vSocketLog;
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

	void log_socket_error(std::string sError);
	void log_result_error(std::string sError);
	void log_result_ok(uint64_t iActualDiff);

	void on_sock_ready();
	void on_sock_error(const msgstruct::sock_err &err);
	void on_pool_have_job(const msgstruct::pool_job& oPoolJob);
	void on_miner_result(const msgstruct::job_result& oResult);
	bool is_pool_live();
	void eval_pool_choice();

	inline size_t sec_to_ticks(size_t sec) { return sec * (1000 / iTickTime); }
};

