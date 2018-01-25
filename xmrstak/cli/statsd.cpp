//
// Created by Snow Flake on 1/20/18.
//

#include "statsd.hpp"
#include "includes/StatsdClient.hpp"
#include "xmrstak/system_constants.hpp"
#include <memory>

namespace statsd {

	std::shared_ptr<Statsd::StatsdClient> get_client() {
		return std::shared_ptr<Statsd::StatsdClient>(
				new Statsd::StatsdClient(
						system_constants::get_statsd_address(),
						system_constants::get_statsd_port(),
						system_constants::get_statsd_prefix()
				)
		);
	}

	void statsd_increment(const std::string &key, const float frequency) {
		auto client = get_client();
		client->increment(key, frequency);
		#ifdef DEBUG_LOGGING
		std::cout << __FILE__ << ":" << __LINE__ << ":[increment]:" << key << std::endl;
		#endif
	}

	void statsd_decrement(const std::string &key, const float frequency) {
		auto client = get_client();
		client->decrement(key, frequency);
		#ifdef DEBUG_LOGGING
		std::cout << __FILE__ << ":" << __LINE__ << ":[decrement]:" << key << std::endl;
		#endif
	}

	void statsd_count(const std::string &key, const int delta, const float frequency) {
		auto client = get_client();
		client->count(key, delta, frequency);
		#ifdef DEBUG_LOGGING
		std::cout << __FILE__ << ":" << __LINE__ << ":[count]:" << key << "=" << delta << std::endl;
		#endif
	}

	void statsd_gauge(const std::string &key, const unsigned int value, const float frequency) {
		auto client = get_client();
		client->gauge(key, value, frequency);
		#ifdef DEBUG_LOGGING
		std::cout << __FILE__ << ":" << __LINE__ << ":[gauge]:" << key << "=" << value << std::endl;
		#endif
	}

	void statsd_timing(const std::string &key, const unsigned int ms, const float frequency) {
		auto client = get_client();
		client->timing(key, ms, frequency);
		#ifdef DEBUG_LOGGING
		std::cout << __FILE__ << ":" << __LINE__ << ":[timing]:" << key << "=" << ms << "(ms)" << std::endl;
		#endif
	}
	
}