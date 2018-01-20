#ifndef STATSD_CLIENT_HPP
#define STATSD_CLIENT_HPP

#include "UDPSender.hpp"
#include <cstdlib>
#include <experimental/optional>
#include <string>

namespace Statsd {
/*!
 *
 * Statsd client
 *
 * This is the Statsd client, exposing the classic methods
 * and relying on a UDP sender for the actual sending.
 *
 * The sampling frequency can be input, as well as the
 * batching size. The latter is optional and shall not be
 * set if batching is not desired.
 *
 */
	class StatsdClient {
	public:

		//!@name Constructor and destructor
		//!@{

		//! Constructor
		StatsdClient(
				const std::string &host,
				const uint16_t port,
				const std::string &prefix,
				const std::experimental::optional<uint64_t> batchsize = std::experimental::nullopt) noexcept;

		//!@}

		//!@name Methods
		//!@{

		//! Sets a configuration { host, port, prefix }
		void setConfig(const std::string &host, const uint16_t port, const std::string &prefix) noexcept;

		//! Returns the error message as an optional std::string
		std::experimental::optional<std::string> errorMessage() const noexcept;

		//! Increments the key, at a given frequency rate
		void increment(const std::string &key, const float frequency = 1.0f) const noexcept;

		//! Increments the key, at a given frequency rate
		void decrement(const std::string &key, const float frequency = 1.0f) const noexcept;

		//! Adjusts the specified key by a given delta, at a given frequency rate
		void count(const std::string &key, const int delta, const float frequency = 1.0f) const noexcept;

		//! Records a gauge for the key, with a given value, at a given frequency rate
		void gauge(const std::string &key, const unsigned int value, const float frequency = 1.0f) const noexcept;

		//! Records a timing for a key, at a given frequency
		void timing(const std::string &key, const unsigned int ms, const float frequency = 1.0f) const noexcept;

		//! Send a value for a key, according to its type, at a given frequency
		void send(const std::string &key, const int value, const std::string &type, const float frequency = 1.0f) const noexcept;

		//!@}

	private:

		//! The prefix to be used for metrics
		std::string m_prefix;

		//! The UDP sender to be used for actual sending
		mutable UDPSender m_sender;
	};

}

#endif
