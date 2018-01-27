#pragma once

#include <unistd.h>
#include <vector>
#include <cstdint>
#include "sys_auto_adjust.hpp"

// define xmr settings
#define MONERO_MEMORY 2097152llu
#define MONERO_MASK 0x1FFFF0
#define MONERO_ITER 0x80000


namespace xmrstak
{
	namespace cpu {

		struct auto_thd_cfg {
			int low_power_mode;
			long long affine_to_cpu;
		};

		class auto_threads {
		public:

			const size_t hashMemSize;
			const size_t halfHashMemSize;
			const uint32_t processors_count;
			const uint32_t cache_l2;
			const uint32_t cache_l3;
			std::vector<auto_thd_cfg> configs;

			auto_threads();

		private:
			bool is_old_amd(int32_t *cpu_info);
		};

	}
}