#pragma once

#include "jconf.hpp"

#include "xmrstak/misc/console.hpp"
#include "xmrstak/jconf.hpp"
#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/params.hpp"
#include "c_cryptonight/cryptonight.hpp"
#include <string>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <vector>


namespace xmrstak {
	namespace cpu {

		struct auto_thd_cfg {
			int low_power_mode;
			bool no_prefetch;
			long long affine_to_cpu;
		};

		class autoAdjust {
		public:

			const size_t hashMemSize;
			const size_t halfHashMemSize;
			const uint32_t processors_count;
			std::vector<auto_thd_cfg> configs;

			autoAdjust();
		private:
			int32_t calc_genuine_intel(int32_t *cpu_info);
			int32_t calc_authentic_amd(int32_t *cpu_info);
			bool is_old_amd(int32_t *cpu_info);
		};

	} // namespace cpu
} // namepsace xmrstak