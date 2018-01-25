#ifndef XMR_SLIM_SYSTEM_CONSTANTS_HPP
#define XMR_SLIM_SYSTEM_CONSTANTS_HPP

#include <inttypes.h>
#include <string>
#include <iostream>

namespace system_constants {

	inline std::string get_version_str() {
		return std::string("xmr-stak/2.2.0/2ae7260/master/mac/cpu/monero/20");
	}

	inline std::string get_version_str_short() {
		return std::string("xmr-stak 2.2.0 2ae7260");
	}

	enum slow_mem_cfg {
		always_use,
		no_mlck,
		print_warning,
		never_use,
		unknown_value
	};

	inline const std::string get_statsd_address() { return std::string(CONFIG_STATSD_ADDRESS); }
	inline const uint16_t get_statsd_port() { return CONFIG_STATSD_PORT; }
	inline const std::string get_statsd_prefix() { return CONFIG_STATSD_PREFIX; }
	inline const std::string get_statsd_machine_id() { return CONFIG_STATSD_MACHINE_ID; }

	inline const std::string get_pool_pool_address() { return std::string(CONFIG_POOL_POOL_ADDRESS); }
	inline const std::string get_pool_wallet_address() { return std::string(CONFIG_POOL_WALLET_ADDRESS); }
	inline const std::string get_pool_pool_password() { return std::string(CONFIG_POOL_POOL_PASSWORD); }

	inline uint64_t GetVerboseLevel() { return CONFIG_VERBOSE_LEVEL; }

	inline uint64_t GetAutohashTime() { return CONFIG_H_PRINT_TIME; }

	inline uint64_t GetCallTimeout() { return CONFIG_CALL_TIMEOUT; }

	inline uint64_t GetNetRetry() { return CONFIG_RETRY_TIME; }

	inline uint64_t GetGiveUpLimit() { return CONFIG_GIVEUP_LIMIT; }

	inline bool HaveHardwareAes() { return CONFIG_AES_OVERRIDE; }

	inline slow_mem_cfg GetSlowMemSetting() { return CONFIG_USE_SLOW_MEMORY; }
}

#endif //XMR_SLIM_SYSTEM_CONSTANTS_HPP