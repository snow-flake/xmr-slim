#pragma once

#include "xmrstak/params.hpp"
#include "autoAdjust.hpp"
#include <stdlib.h>
#include <string>

namespace xmrstak {
	namespace cpu {
		namespace jconf {

			struct thd_cfg {
				int iMultiway;  // low_power_mode
				bool bNoPrefetch; // no_prefetch
				long long iCpuAff; // affine_to_cpu
				thd_cfg() : iMultiway(1), bNoPrefetch(true), iCpuAff(-1) {}
			};


			static inline bool GetThreadConfig(size_t id, thd_cfg &cfg) {
				auto auto_config = auto_threads().configs[id];

				cfg.iMultiway = auto_config.low_power_mode;
				cfg.bNoPrefetch = auto_config.no_prefetch;
				cfg.iCpuAff = auto_config.affine_to_cpu;

				return true;
			}

			static inline size_t GetThreadCount() {
				return auto_threads().processors_count;
			}

		}
	}
}