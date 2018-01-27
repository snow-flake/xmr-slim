#include "autoAdjust.hpp"

#include <string.h>
#include <cpuid.h>
#include <iostream>
#include <exception>
#include <vector>


// Mask bits between h and l and return the value
// This enables us to put in values exactly like in the manual
// For example EBX[31:22] is get_masked(cpu_info[1], 31, 22)
inline int32_t get_masked(int32_t val, int32_t h, int32_t l) {
	val &= (0x7FFFFFFF >> (31 - (h - l))) << l;
	return val >> l;
}

void cpuid(uint32_t eax, int32_t ecx, int32_t val[4])
{
	memset(val, 0, sizeof(int32_t)*4);
	__cpuid_count(eax, ecx, val[0], val[1], val[2], val[3]);
}

void check_cpu_features(bool & haveAes, bool & haveSse2)
{
	constexpr int AESNI_BIT = 1 << 25;
	constexpr int SSE2_BIT = 1 << 26;
	int32_t cpu_info[4];
	bool bHaveSse2;

	cpuid(1, 0, cpu_info);

	haveAes = (cpu_info[2] & AESNI_BIT) != 0;
	haveSse2 = (cpu_info[3] & SSE2_BIT) != 0;
}

void parse_config() {
	bool haveAes = false;
	bool haveSse2 = false;
	check_cpu_features(haveAes, haveSse2);

	if(!haveSse2) {
		throw new std::runtime_error("CPU support of SSE2 is required.");
	}
	if(!haveAes) {
		throw new std::runtime_error("WARNING: CPU support of AES is not available.");
	}
}


xmrstak::cpu::auto_threads::auto_threads() :
		hashMemSize(MONERO_MEMORY),
		halfHashMemSize(MONERO_MEMORY / 2u),
		processors_count(CONFIG_SYSTEM_NPROC),
		cache_l2(CONFIG_SYSTEM_CACHE_L2),
		cache_l3(CONFIG_SYSTEM_CACHE_L3),
		configs(CONFIG_SYSTEM_NPROC) {
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: hashMemSize      = " << hashMemSize << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: halfHashMemSize  = " << halfHashMemSize << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: processors_count = " << processors_count << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: cache_l2         = " << cache_l2 << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: cache_l3         = " << cache_l3 << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " auto_threads: configs.size     = " << configs.size() << std::endl;

	parse_config();

	int32_t cpu_info[4];
	const int32_t L3KB_size = cache_l3;
	bool old_amd = false;
	char cpustr[13] = {0};

	cpuid(0, 0, cpu_info);
	memcpy(cpustr, &cpu_info[1], 4);
	memcpy(cpustr + 4, &cpu_info[3], 4);
	memcpy(cpustr + 8, &cpu_info[2], 4);

	if (strcmp(cpustr, "GenuineIntel") == 0) {
		//		L3KB_size = calc_genuine_intel(cpu_info);
	} else if (strcmp(cpustr, "AuthenticAMD") == 0) {
		//		L3KB_size = calc_authentic_amd(cpu_info);
		old_amd = is_old_amd(cpu_info);
	} else {
		std::cout << __FILE__ << ":" << __LINE__ << ":" << "Autoconf failed: Unknown CPU type: " << cpustr << std::endl;
		throw new std::runtime_error("Failed to detect CPU type");
	}

	const bool isLessThanMem = L3KB_size < halfHashMemSize;
	const bool isMoreThan100u = L3KB_size > (halfHashMemSize * 100u);
	if (isLessThanMem) {
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf failed: L3 size sanity check failed " << L3KB_size << " KB" << " vs halfHashMemSize=" << halfHashMemSize << std::endl;
		// throw new std::runtime_error("L3 size sanity check failed");
	} else if (isMoreThan100u) {
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf failed: L3 size sanity check failed " << L3KB_size << " KB" << " vs halfHashMemSize * 100u = " << halfHashMemSize * 100u << std::endl;
		// throw new std::runtime_error("L3 size sanity check failed");
	}


	uint32_t affine_to_cpu = 0;
	const int32_t requred_cache_per_thread = 1024 * 1024 * 2;
	const int32_t available_cache_per_thread = cache_l3 / (requred_cache_per_thread * processors_count);
	const int32_t low_power_mode = available_cache_per_thread > 5 ? 5 : available_cache_per_thread;

	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf processors_count           = " << processors_count << " on Linux" << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf requred_cache_per_thread   = " << requred_cache_per_thread << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf available_cache_per_thread = " << available_cache_per_thread << std::endl;
	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf low_power_mode             = " << low_power_mode << " on Linux" << std::endl;

	for (uint32_t i = 0; i < processors_count; i++) {
		auto_thd_cfg config = auto_thd_cfg();
		config.low_power_mode = low_power_mode;
		config.affine_to_cpu = affine_to_cpu;
		configs[i] = config;

		if (old_amd) {
			affine_to_cpu += 2;
			if (affine_to_cpu >= processors_count)
				affine_to_cpu = 1;
		} else {
			affine_to_cpu++;
		}
	}

	std::cout << "#pragma once" << std::endl;
	std::cout << "#define CONFIG_MINE_PARALLEL_LEVEL " << low_power_mode << std::endl;
	std::cout << "#define CONFIG_MINE_CPU_COUNT " << CONFIG_SYSTEM_NPROC << std::endl;
	std::cout << "#define CONFIG_MINE_CPU_IDS ";
	for (uint32_t i = 0; i < processors_count; i++) {
		std::cout << configs[i].affine_to_cpu;
		if (i < processors_count-1) {
			std::cout << ",";
		}
	}
	std::cout << "" << std::endl;
	std::cout << "" << std::endl;

	std::cout << "export CONFIG_MINE_PARALLEL_LEVEL=" << low_power_mode << std::endl;
	std::cout << "export CONFIG_MINE_CPU_COUNT=" << CONFIG_SYSTEM_NPROC << std::endl;
	std::cout << "export CONFIG_MINE_CPU_IDS=\"";
	for (uint32_t i = 0; i < processors_count; i++) {
		std::cout << configs[i].affine_to_cpu;
		if (i < processors_count-1) {
			std::cout << ",";
		}
	}
	std::cout << "\"" << std::endl;
}


bool xmrstak::cpu::auto_threads::is_old_amd(int32_t *cpu_info) {
	cpuid(0x80000006, 0, cpu_info);
	cpuid(1, 0, cpu_info);
	if (get_masked(cpu_info[0], 11, 8) < 0x17) //0x17h is Zen
		return true;
	return false;
}

