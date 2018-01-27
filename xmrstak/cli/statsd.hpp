//
// Created by Snow Flake on 1/20/18.
//

#ifndef XMR_STAK_STATSD_H
#define XMR_STAK_STATSD_H

#include <string>

namespace statsd {

	//! Increments the key, at a given frequency rate
	void statsd_increment(const std::string &key, const float frequency = 1.0f);

	//! Increments the key, at a given frequency rate
	void statsd_decrement(const std::string &key, const float frequency = 1.0f);

	//! Adjusts the specified key by a given delta, at a given frequency rate
	void statsd_count(const std::string &key, const int delta, const float frequency = 1.0f);

	//! Records a gauge for the key, with a given value, at a given frequency rate
	void statsd_gauge(const std::string &key, const unsigned int value, const float frequency = 1.0f);

	//! Records a timing for a key, at a given frequency
	void statsd_timing(const std::string &key, const unsigned int ms, const float frequency = 1.0f);

	// Record the miner connected to the pool
	void log_login(const std::string & address, const std::string & password, const std::string & user_agent);

	// Record the miner received a new task
	void log_job(const std::string & miner_id, const std::string & job_id, const std::string & target, const std::string & blob);

	// Record the miner finished a task
	void log_result(const std::string & miner_id, const std::string & job_id, const std::string & target, const std::string & blob, const std::string & result, const std::string & nonce);

}

#endif //XMR_STAK_STATSD_H
