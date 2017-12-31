/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "jconf.hpp"

#include "xmrstak/misc/console.hpp"
#include <cpuid.h>


jconf::jconf() {}

void jconf::cpuid(uint32_t eax, int32_t ecx, int32_t val[4])
{
	memset(val, 0, sizeof(int32_t)*4);
	__cpuid_count(eax, ecx, val[0], val[1], val[2], val[3]);
}

void jconf::check_cpu_features(bool & haveAes, bool & haveSse2)
{
	constexpr int AESNI_BIT = 1 << 25;
	constexpr int SSE2_BIT = 1 << 26;
	int32_t cpu_info[4];
	bool bHaveSse2;

	cpuid(1, 0, cpu_info);

	haveAes = (cpu_info[2] & AESNI_BIT) != 0;
	haveSse2 = (cpu_info[3] & SSE2_BIT) != 0;
}

bool jconf::parse_config() {
	bool haveAes = false;
	bool haveSse2 = false;
	check_cpu_features(haveAes, haveSse2);

	if(!haveSse2) {
		printer::inst()->print_msg(L0, "CPU support of SSE2 is required.");
		return false;
	}
	if(!haveAes) {
		printer::inst()->print_msg(L0, "WARNING: CPU support of AES is not available.");
	}
	if(GetCallTimeout() == 0 || GetNetRetry() == 0) {
		printer::inst()->print_msg(L0, "Invalid config file. call_timeout and retry_time need to be positive integers.");
		return false;
	}

	if(GetCallTimeout() < 2 || GetNetRetry() < 2) {
		printer::inst()->print_msg(L0, "Invalid config file. call_timeout and retry_time need to be larger than 1 second.");
		return false;
	}

	printer::inst()->set_verbose_level(GetVerboseLevel());

	if(GetSlowMemSetting() == unknown_value) {
		printer::inst()->print_msg(L0, "Invalid config file. use_slow_memory must be \"always\", \"no_mlck\", \"warn\" or \"never\"");
		return false;
	}

	return true;
}
