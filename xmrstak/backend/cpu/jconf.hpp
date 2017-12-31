#pragma once

#include "xmrstak/params.hpp"

#include <stdlib.h>
#include <string>

namespace xmrstak
{
namespace cpu
{

class jconf
{
public:
	static jconf* inst()
	{
		if (oInst == nullptr) oInst = new jconf;
		return oInst;
	};

	bool parse_config(const char* sFilename = params::inst().configFileCPU.c_str());

	struct thd_cfg {
		int iMultiway;  // low_power_mode
		bool bNoPrefetch; // no_prefetch
		long long iCpuAff; // affine_to_cpu
		thd_cfg(): iMultiway(1), bNoPrefetch(true), iCpuAff(-1)  {}
	};

	size_t GetThreadCount();
	bool GetThreadConfig(size_t id, thd_cfg &cfg);
	bool NeedsAutoconf();

private:
	jconf();
	static jconf* oInst;

	struct opaque_private;
	opaque_private* prv;
};

} // namespace cpu
} // namepsace xmrstak
