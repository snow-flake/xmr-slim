#include "autoAdjust.hpp"

//#include "jconf.hpp"

//#include "xmrstak/misc/console.hpp"
#include "xmrstak/jconf.hpp"
//#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/params.hpp"
#include "c_cryptonight/cryptonight.hpp"
#include <string>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <vector>
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
		processors_count(sysconf(_SC_NPROCESSORS_ONLN)),
		configs(sysconf(_SC_NPROCESSORS_ONLN)) {
	int32_t cpu_info[4];
	int32_t L3KB_size = 0;
	bool old_amd = false;
	char cpustr[13] = {0};

	::jconf::cpuid(0, 0, cpu_info);
	memcpy(cpustr, &cpu_info[1], 4);
	memcpy(cpustr + 4, &cpu_info[3], 4);
	memcpy(cpustr + 8, &cpu_info[2], 4);

	if (strcmp(cpustr, "GenuineIntel") == 0) {
		L3KB_size = calc_genuine_intel(cpu_info);
	} else if (strcmp(cpustr, "AuthenticAMD") == 0) {
		L3KB_size = calc_authentic_amd(cpu_info);
		old_amd = is_old_amd(cpu_info);
	} else {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << "Autoconf failed: Unknown CPU type: " << cpustr << std::endl;
		throw new std::runtime_error("Failed to detect CPU type");
	}

	const bool isLessThanMem = L3KB_size < halfHashMemSize;
	const bool isMoreThan100u = L3KB_size > (halfHashMemSize * 100u);
	if (isLessThanMem || isMoreThan100u) {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << "Autoconf failed: L3 size sanity check failed " << L3KB_size << " KB" << std::endl;
		// throw new std::runtime_error("L3 size sanity check failed");
	}

	std::cout << __FILE__ << ":" << __LINE__ << ":" << " Autoconf core count detected as " << processors_count << " on Linux" << std::endl;

	uint32_t aff_id = 0;
	for (uint32_t i = 0; i < processors_count; i++) {
		bool double_mode;

		if (L3KB_size <= 0)
			break;

		double_mode = L3KB_size / hashMemSize > (int32_t) (processors_count - i);

		auto_thd_cfg config = auto_thd_cfg();
		config.low_power_mode = double_mode;
		config.no_prefetch = true;
		config.affine_to_cpu = aff_id;
		configs[i] = config;

		if (old_amd) {
			aff_id += 2;
			if (aff_id >= processors_count)
				aff_id = 1;
		} else {
			aff_id++;
		}

		if (double_mode)
			L3KB_size -= hashMemSize * 2u;
		else
			L3KB_size -= hashMemSize;
	}
}

int32_t xmrstak::cpu::auto_threads::calc_genuine_intel(int32_t *cpu_info) {
	::jconf::cpuid(4, 3, cpu_info);

	if (get_masked(cpu_info[0], 7, 5) != 3) {
		std::cerr << __FILE__ << ":" << __LINE__ << ":" << "Autoconf failed: Couldn't find L3 cache page" << std::endl;
		throw new std::runtime_error("Failed to detect CPU type");
	}

	int32_t l3cache = ((get_masked(cpu_info[1], 31, 22) + 1) * (get_masked(cpu_info[1], 21, 12) + 1) * (get_masked(cpu_info[1], 11, 0) + 1) * (cpu_info[2] + 1)) / halfHashMemSize;
	return l3cache;
}

int32_t xmrstak::cpu::auto_threads::calc_authentic_amd(int32_t *cpu_info) {
	::jconf::cpuid(0x80000006, 0, cpu_info);
	int32_t l3cache = get_masked(cpu_info[3], 31, 18) * 512;
	return l3cache;
}

bool xmrstak::cpu::auto_threads::is_old_amd(int32_t *cpu_info) {
	::jconf::cpuid(0x80000006, 0, cpu_info);
	::jconf::cpuid(1, 0, cpu_info);
	if (get_masked(cpu_info[0], 11, 8) < 0x17) //0x17h is Zen
		return true;
	return false;
}

