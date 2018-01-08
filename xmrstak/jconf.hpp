#pragma once

#include "xmrstak/misc/environment.hpp"

#include <stdlib.h>
#include <string>
#include <iostream>

class jconf
{
public:
	static jconf* inst()
	{
		auto& env = xmrstak::environment::inst();
		if(env.pJconfConfig == nullptr)
			env.pJconfConfig = new jconf;
		return env.pJconfConfig;
	};

	bool parse_config();

	struct pool_cfg {
		const char* sPoolAddr;
		const char* sWalletAddr;
		const char* sPasswd;
		const bool nicehash;
		const bool tls;
		const char* tls_fingerprint;
		const size_t raw_weight;
		const double weight;
		pool_cfg():
			sPoolAddr(CONFIG_POOL_POOL_ADDRESS),
			sWalletAddr(CONFIG_POOL_WALLET_ADDRESS),
			sPasswd(CONFIG_POOL_POOL_PASSWORD),
			nicehash(CONFIG_POOL_USE_NICEHASH),
			tls(CONFIG_POOL_USE_TLS),
			tls_fingerprint(CONFIG_POOL_TLS_FINGERPRINT),
			raw_weight(CONFIG_POOL_POOL_WEIGHT),
			weight(CONFIG_POOL_POOL_WEIGHT)
		{
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_ADDRESS:    " << sPoolAddr << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_WALLET_ADDRESS:  " << sWalletAddr << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_PASSWORD:   " << sPasswd << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_USE_NICEHASH:    " << nicehash << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_USE_TLS:         " << tls << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_TLS_FINGERPRINT: " << tls_fingerprint << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_WEIGHT:     " << raw_weight << std::endl;
			 std::cout << __FILE__ << ":" << __LINE__ << ":" << " pool_cfg: CONFIG_POOL_POOL_WEIGHT:     " << weight << std::endl;
		}
	};

	enum slow_mem_cfg {
		always_use,
		no_mlck,
		print_warning,
		never_use,
		unknown_value
	};

	inline bool TlsSecureAlgos() { return CONFIG_TLS_SECURE_ALGO; }
	inline uint64_t GetVerboseLevel() { return CONFIG_VERBOSE_LEVEL; }
	inline bool PrintMotd() { return CONFIG_PRINT_MOTD; }
	inline uint64_t GetAutohashTime() { return CONFIG_H_PRINT_TIME; }
	inline const char* GetOutputFile() { return CONFIG_OUTPUT_FILE; }
	inline uint64_t GetCallTimeout() { return CONFIG_CALL_TIMEOUT; }
	inline uint64_t GetNetRetry() { return CONFIG_RETRY_TIME; }
	inline uint64_t GetGiveUpLimit() { return CONFIG_GIVEUP_LIMIT; }
	inline bool DaemonMode() { return CONFIG_DAEMON_MODE; }
	inline bool HaveHardwareAes() { return CONFIG_AES_OVERRIDE; }
	inline slow_mem_cfg GetSlowMemSetting() { return CONFIG_USE_SLOW_MEMORY; }

	static void cpuid(uint32_t eax, int32_t ecx, int32_t val[4]);

private:
	jconf();

	void check_cpu_features(bool & haveAes, bool & haveSse2);
};
