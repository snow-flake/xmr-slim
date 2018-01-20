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

}

#endif //XMR_STAK_STATSD_H
