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
#include "xmrstak/backend/miner_work.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/backend/backendConnector.hpp"
#include "xmrstak/jconf.hpp"
#include "xmrstak/misc/console.hpp"
#include "xmrstak/params.hpp"
#include "xmrstak/version.hpp"
#include "xmrstak/misc/utility.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <time.h>
#include <iostream>

#ifndef CONF_NO_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

void do_benchmark();

void help()
{
	using namespace std;
	using namespace xmrstak;

	cout<<"Usage: "<<params::inst().binaryName<<" [OPTION]..."<<endl;
	cout<<" "<<endl;
	cout<<"  -h, --help            show this help"<<endl;
	cout<<"  -v, --version         show version number"<<endl;
	cout<<"  -V, --version-long    show long version number"<<endl;
	cout<< "Version: " << get_version_str_short() << endl;
}

int main(int argc, char *argv[])
{
#ifndef CONF_NO_TLS
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	OpenSSL_add_all_digests();
#endif

	srand(time(0));

	using namespace xmrstak;

	std::string pathWithName(argv[0]);
	std::string seperator("/");
	auto pos = pathWithName.rfind(seperator);

	if(pos == std::string::npos)
	{
		// try windows "\"
		seperator = "\\";
		pos = pathWithName.rfind(seperator);
	}
	params::inst().binaryName = std::string(pathWithName, pos + 1, std::string::npos);
	if(params::inst().binaryName.compare(pathWithName) != 0)
	{
		params::inst().executablePrefix = std::string(pathWithName, 0, pos);
		params::inst().executablePrefix += seperator;
	}

	for(size_t i = 1; i < argc; ++i)
	{
		std::string opName(argv[i]);
		if(opName.compare("-h") == 0 || opName.compare("--help") == 0)
		{
			help();
			win_exit(0);
			return 0;
		}
		if(opName.compare("-v") == 0 || opName.compare("--version") == 0)
		{
			std::cout<< "Version: " << get_version_str_short() << std::endl;
			win_exit();
			return 0;
		}
		else if(opName.compare("-V") == 0 || opName.compare("--version-long") == 0)
		{
			std::cout<< "Version: " << get_version_str() << std::endl;
			win_exit();
			return 0;
		}
		else
		{
			printer::inst()->print_msg(L0, "Parameter unknown '%s'",argv[i]);
			win_exit();
			return 1;
		}
	}

	if(!jconf::inst()->parse_config())
	{
		win_exit();
		return 1;
	}

	if (!BackendConnector::self_test())
	{
		win_exit();
		return 1;
	}

	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_str(get_version_str_short().c_str());
	printer::inst()->print_str("\n\n");
	printer::inst()->print_str("You can use following keys to display reports:\n");
	printer::inst()->print_str("'h' - hashrate\n");
	printer::inst()->print_str("'r' - results\n");
	printer::inst()->print_str("'c' - connection\n");
	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_msg(L0,"Start mining: MONERO");

	if(strlen(jconf::inst()->GetOutputFile()) != 0) {
		printer::inst()->open_logfile(jconf::inst()->GetOutputFile());
	}

	executor::inst()->ex_start(jconf::inst()->DaemonMode());

	uint64_t lastTime = get_timestamp_ms();
	int key;
	while(true)
	{
		key = get_key();
		switch(key) {
		case 'h':
			executor::inst()->push_event(ex_event(EV_USR_HASHRATE));
			break;
		case 'r':
			executor::inst()->push_event(ex_event(EV_USR_RESULTS));
			break;
		case 'c':
			executor::inst()->push_event(ex_event(EV_USR_CONNSTAT));
			break;
		default:
			break;
		}

		uint64_t currentTime = get_timestamp_ms();

		/* Hard guard to make sure we never get called more than twice per second */
		if( currentTime - lastTime < 500) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500 - (currentTime - lastTime)));
		}
		lastTime = currentTime;
	}

	return 0;
}
