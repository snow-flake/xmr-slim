//
// Created by Snow Flake on 1/20/18.
//

#include "statsd.hpp"
#include "includes/StatsdClient.hpp"
#include "includes/json.hpp"
#include "xmrstak/system_constants.hpp"
#include "includes/date/date.h"
#include <chrono>
#include <memory>

namespace statsd {

	template <class Precision>
	const std::string get_iso_current_timestamp()
	{
		auto now = std::chrono::system_clock::now();
		return date::format("%FT%TZ", date::floor<Precision>(now));
	}


	std::shared_ptr<Statsd::StatsdClient> get_client() {
		return std::shared_ptr<Statsd::StatsdClient>(
				new Statsd::StatsdClient(
						system_constants::get_statsd_address(),
						system_constants::get_statsd_port(),
						system_constants::get_statsd_prefix()
				)
		);
	}


	std::shared_ptr<Statsd::UDPSender> get_logger() {
		std::shared_ptr<Statsd::UDPSender> output = std::shared_ptr<Statsd::UDPSender>(
				new Statsd::UDPSender(
						system_constants::get_logging_address(),
						system_constants::get_logging_port()
				)
		);
		return output;
	}


	void statsd_increment(const std::string &key, const float frequency) {
		auto client = get_client();
		client->increment(key, frequency);
		client->increment(system_constants::get_statsd_machine_id() + key, frequency);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:statsd_increment: key=" << key << std::endl;
#endif
	}


	void statsd_decrement(const std::string &key, const float frequency) {
		auto client = get_client();
		client->decrement(key, frequency);
		client->decrement(system_constants::get_statsd_machine_id() + key, frequency);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:statsd_decrement: key=" << key << std::endl;
#endif
	}


	void statsd_count(const std::string &key, const int delta, const float frequency) {
		auto client = get_client();
		client->count(key, delta, frequency);
		client->count(system_constants::get_statsd_machine_id() + key, delta, frequency);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:statsd_count: key=" << key << ", delta=" << delta << std::endl;
#endif
	}


	void statsd_gauge(const std::string &key, const unsigned int value, const float frequency) {
		auto client = get_client();
		client->gauge(key, value, frequency);
		client->gauge(system_constants::get_statsd_machine_id() + key, value, frequency);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:statsd_gauge: key=" << key << ", value=" << value << std::endl;
#endif
	}


	void statsd_timing(const std::string &key, const unsigned int ms, const float frequency) {
		auto client = get_client();
		client->timing(key, ms, frequency);
		client->timing(system_constants::get_statsd_machine_id() + key, ms, frequency);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:statsd_timing: key=" << key << ", ms=" << ms << std::endl;
#endif
	}


	// Record the miner connected to the pool
	void log_login(const std::string & address, const std::string & password, const std::string & user_agent) {
		auto logger = get_logger();

		nlohmann::json data;
		data["time"] = get_iso_current_timestamp<std::chrono::microseconds>();
		data["machine_id"] = system_constants::get_statsd_machine_id();
		data["event"]["action"] = "login";
		data["event"]["address"] = address;
		data["event"]["password"] = password;
		data["event"]["user_agent"] = user_agent;
		const std::string cmd_buffer = data.dump();
		logger->send(cmd_buffer);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:log_login: send=" << cmd_buffer << std::endl;
#endif
	}


	// Record the miner received a new task
	void log_job(const std::string & miner_id, const std::string & job_id, const std::string & target, const std::string & blob) {
		auto logger = get_logger();

		nlohmann::json data;
		data["time"] = get_iso_current_timestamp<std::chrono::microseconds>();
		data["machine_id"] = system_constants::get_statsd_machine_id();
		data["event"]["action"] = "new_job";
		data["event"]["miner_id"] = miner_id;
		data["event"]["job_id"] = job_id;
		data["event"]["target"] = target;
		data["event"]["blob"] = blob;
		const std::string cmd_buffer = data.dump();
		logger->send(cmd_buffer);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:log_job: send=" << cmd_buffer << std::endl;
#endif
	}


	// Record the miner finished a task
	void log_result(const std::string & miner_id, const std::string & job_id, const std::string & target, const std::string & blob, const std::string & result, const std::string & nonce) {
		auto logger = get_logger();

		nlohmann::json data;
		data["time"] = get_iso_current_timestamp<std::chrono::microseconds>();
		data["machine_id"] = system_constants::get_statsd_machine_id();
		data["event"]["action"] = "submit_job";
		data["event"]["miner_id"] = miner_id;
		data["event"]["job_id"] = job_id;
		data["event"]["target"] = target;
		data["event"]["blob"] = blob;
		data["event"]["result"] = result;
		data["event"]["nonce"] = nonce;
		const std::string cmd_buffer = data.dump();
		logger->send(cmd_buffer);

#ifdef CONFIG_DEBUG_MODE
		std::cout << __FILE__ << ":" << __LINE__ << ":statsd:log_result: send=" << cmd_buffer << std::endl;
#endif
	}

}