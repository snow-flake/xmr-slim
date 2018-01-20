
#include "StatsdClient.hpp"

namespace Statsd {

	StatsdClient::StatsdClient(
			const std::string &host,
			const uint16_t port,
			const std::string &prefix,
			const std::experimental::optional<uint64_t> batchsize) noexcept
			: m_prefix(prefix), m_sender(host, port, batchsize) {
		// Initialize the randorm generator to be used for sampling
		std::srand(time(nullptr));
	}

	void StatsdClient::setConfig(const std::string &host, const uint16_t port, const std::string &prefix) noexcept {
		m_prefix = prefix;
		m_sender.setConfig(host, port);
	}

	std::experimental::optional<std::string> StatsdClient::errorMessage() const noexcept {
		return m_sender.errorMessage();
	}

	void StatsdClient::decrement(const std::string &key, const float frequency) const noexcept {
		return count(key, -1, frequency);
	}

	void StatsdClient::increment(const std::string &key, const float frequency) const noexcept {
		return count(key, 1, frequency);
	}

	void StatsdClient::count(const std::string &key, const int delta, const float frequency) const noexcept {
		return send(key, delta, "c", frequency);
	}

	void StatsdClient::gauge(const std::string &key, const unsigned int value, const float frequency) const noexcept {
		return send(key, value, "g", frequency);
	}

	void StatsdClient::timing(const std::string &key, const unsigned int ms, const float frequency) const noexcept {
		return send(key, ms, "ms", frequency);
	}

	void StatsdClient::send(const std::string &key, const int value, const std::string &type, const float frequency) const noexcept {
		const auto isFrequencyOne = [](const float frequency) noexcept {
			constexpr float epsilon{0.0001f};
			return std::fabs(frequency - 1.0f) < epsilon;
		};

		// Test if one should send or not, according to the frequency rate
		if (!isFrequencyOne(frequency)) {
			if (frequency < static_cast<float>(std::rand()) / RAND_MAX) {
				return;
			}
		}

		// Prepare the buffer, with a sampling rate if specified different from 1.0f
		char buffer[256];
		if (isFrequencyOne(frequency)) {
			// Sampling rate is 1.0f, no need to specify it
			std::snprintf(buffer, sizeof(buffer), "%s%s:%d|%s", m_prefix.c_str(), key.c_str(), value, type.c_str());
		} else {
			// Sampling rate is different from 1.0f, hence specify it
			std::snprintf(buffer, sizeof(buffer), "%s%s:%d|%s|@%.2f", m_prefix.c_str(), key.c_str(), value, type.c_str(), frequency);
		}

		// Send the message via the UDP sender
		m_sender.send(buffer);
	}
}
