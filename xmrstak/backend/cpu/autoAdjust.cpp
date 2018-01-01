#include "autoAdjust.hpp"

#include "xmrstak/jconf.hpp"
#include "c_cryptonight/cryptonight.hpp"
#include <string.h>


// Mask bits between h and l and return the value
// This enables us to put in values exactly like in the manual
// For example EBX[31:22] is get_masked(cpu_info[1], 31, 22)
inline int32_t get_masked(int32_t val, int32_t h, int32_t l) {
	val &= (0x7FFFFFFF >> (31 - (h - l))) << l;
	return val >> l;
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

	int32_t cpu_info[4];
	const int32_t L3KB_size = cache_l3;
	bool old_amd = false;
	char cpustr[13] = {0};

	::jconf::cpuid(0, 0, cpu_info);
	memcpy(cpustr, &cpu_info[1], 4);
	memcpy(cpustr + 4, &cpu_info[3], 4);
	memcpy(cpustr + 8, &cpu_info[2], 4);

	if (strcmp(cpustr, "GenuineIntel") == 0) {
		//		L3KB_size = calc_genuine_intel(cpu_info);
	} else if (strcmp(cpustr, "AuthenticAMD") == 0) {
		//		L3KB_size = calc_authentic_amd(cpu_info);
		old_amd = is_old_amd(cpu_info);
	} else {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << "Autoconf failed: Unknown CPU type: " << cpustr << std::endl;
		throw new std::runtime_error("Failed to detect CPU type");
	}

	const bool isLessThanMem = L3KB_size < halfHashMemSize;
	const bool isMoreThan100u = L3KB_size > (halfHashMemSize * 100u);
	if (isLessThanMem) {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << " Autoconf failed: L3 size sanity check failed " << L3KB_size << " KB" << " vs halfHashMemSize=" << halfHashMemSize << std::endl;
		// throw new std::runtime_error("L3 size sanity check failed");
	} else if (isMoreThan100u) {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << " Autoconf failed: L3 size sanity check failed " << L3KB_size << " KB" << " vs halfHashMemSize * 100u = " << halfHashMemSize * 100u << std::endl;
		// throw new std::runtime_error("L3 size sanity check failed");
	}

	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf core count detected as " << processors_count << " on Linux" << std::endl;

	uint32_t affine_to_cpu = 0;
	int32_t available_cache = L3KB_size;

	for (uint32_t i = 0; i < processors_count; i++) {
		const bool double_mode = available_cache / hashMemSize > (int32_t) (processors_count - i);
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf thread config i=" << i << ": available_cache=" << available_cache << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf thread config i=" << i << ": low_power_mode=" << double_mode << std::endl;
		std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf thread config i=" << i << ": affine_to_cpu=" << affine_to_cpu << std::endl;

		if (available_cache <= 0) {
			break;
		}

		auto_thd_cfg config = auto_thd_cfg();
		config.low_power_mode = 0; // double_mode;
		config.no_prefetch = true;
		config.affine_to_cpu = affine_to_cpu;
		configs[i] = config;


		if (old_amd) {
			affine_to_cpu += 2;
			if (affine_to_cpu >= processors_count)
				affine_to_cpu = 1;
		} else {
			affine_to_cpu++;
		}

		if (double_mode)
			available_cache -= hashMemSize * 2u;
		else
			available_cache -= hashMemSize;
	}
}


bool xmrstak::cpu::auto_threads::is_old_amd(int32_t *cpu_info) {
	::jconf::cpuid(0x80000006, 0, cpu_info);
	::jconf::cpuid(1, 0, cpu_info);
	if (get_masked(cpu_info[0], 11, 8) < 0x17) //0x17h is Zen
		return true;
	return false;
}

