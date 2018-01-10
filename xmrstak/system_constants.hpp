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

	inline const char *config_pool_pool_address() { return CONFIG_POOL_POOL_ADDRESS; }

	inline const char *config_pool_wallet_address() { return CONFIG_POOL_WALLET_ADDRESS; }

	inline const char *config_pool_pool_password() { return CONFIG_POOL_POOL_PASSWORD; }

	inline const bool config_pool_use_tls() { return CONFIG_POOL_USE_TLS; }

	inline const char *config_pool_tls_fingerprint() { return CONFIG_POOL_TLS_FINGERPRINT; }

	inline const int config_pool_pool_weight() { return CONFIG_POOL_POOL_WEIGHT; }

	inline bool TlsSecureAlgos() { return CONFIG_TLS_SECURE_ALGO; }

	inline uint64_t GetVerboseLevel() { return CONFIG_VERBOSE_LEVEL; }

	inline bool PrintMotd() { return CONFIG_PRINT_MOTD; }

	inline uint64_t GetAutohashTime() { return CONFIG_H_PRINT_TIME; }

	inline uint64_t GetCallTimeout() { return CONFIG_CALL_TIMEOUT; }

	inline uint64_t GetNetRetry() { return CONFIG_RETRY_TIME; }

	inline uint64_t GetGiveUpLimit() { return CONFIG_GIVEUP_LIMIT; }

	inline bool DaemonMode() { return CONFIG_DAEMON_MODE; }

	inline bool HaveHardwareAes() { return CONFIG_AES_OVERRIDE; }

	inline slow_mem_cfg GetSlowMemSetting() { return CONFIG_USE_SLOW_MEMORY; }

	inline void print_system_config() {
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_ADDRESS:    " << CONFIG_POOL_POOL_ADDRESS << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_WALLET_ADDRESS:  " << CONFIG_POOL_WALLET_ADDRESS << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_PASSWORD:   " << CONFIG_POOL_POOL_PASSWORD << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_USE_TLS:         " << CONFIG_POOL_USE_TLS << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_TLS_FINGERPRINT: " << CONFIG_POOL_TLS_FINGERPRINT << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_WEIGHT:     " << CONFIG_POOL_POOL_WEIGHT << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_WEIGHT:     " << CONFIG_POOL_POOL_WEIGHT << std::endl;
	}
}

#endif //XMR_SLIM_SYSTEM_CONSTANTS_HPP