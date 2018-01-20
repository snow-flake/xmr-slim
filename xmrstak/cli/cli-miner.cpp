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

#include "xmrstak/misc/executor.hpp"
#include "xmrstak/system_constants.hpp"
#include "xmrstak/backend/cpu/minethd.hpp"
#include "xmrstak/net/time_utils.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <time.h>

int main(int argc, char *argv[])
{
	srand(time(0));

	for(size_t i = 1; i < argc; ++i) {
		std::string opName(argv[i]);
		if(opName.compare("--help") == 0) {
			std::cout <<"Usage: xmr-stak [OPTION]..."<< std::endl;
			std::cout <<" "<< std::endl;
			std::cout <<"  --help            show this help"<< std::endl;
			std::cout <<"  --version         show version number"<< std::endl;
			std::cout <<"  --version-long    show long version number"<< std::endl;
			std::cout << "Version: " << system_constants::get_version_str_short() <<  std::endl;
			return 0;
		} else if(opName.compare("--version") == 0) {
			std::cout<< "Version: " << system_constants::get_version_str_short() << std::endl;
			return 0;
		}
		else if(opName.compare("--version-long") == 0) {
			std::cout<< "Version: " << system_constants::get_version_str() << std::endl;
			return 0;
		} else {
			std::cout << "Parameter unknown '%s'" << argv[i] << std::endl;
			return 1;
		}
	}

	if (!xmrstak::cpu::minethd::self_test()) {
		return 1;
	}

	std::cout << "-------------------------------------------------------------------" << std::endl;
	std::cout << "Version: " << system_constants::get_version_str_short() << std::endl;
	std::cout << "Start mining: MONERO" << std::endl;
	std::cout << std::endl;

	executor::inst()->ex_main();
	return 0;
}
