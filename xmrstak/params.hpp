#pragma once

#include "xmrstak/misc/environment.hpp"

#include <string>

namespace xmrstak
{
struct params
{
	static inline params& inst()
	{
		auto& env = environment::inst();
		if(env.pParams == nullptr)
			env.pParams = new params;
		return *env.pParams;
	}

	std::string executablePrefix;
	std::string binaryName;

	params() :
		binaryName("xmr-stak"),
		executablePrefix("")
	{}
};

} // namepsace xmrstak
