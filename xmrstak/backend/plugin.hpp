#pragma once

#include "xmrstak/misc/environment.hpp"
#include "xmrstak/params.hpp"

#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include "iBackend.hpp"
#include <iostream>

#ifndef USE_PRECOMPILED_HEADERS
#		include <sys/types.h>
#		include <dlfcn.h>
#	include <iostream>
#endif

namespace xmrstak
{

struct plugin
{

	plugin(const std::string backendName, const std::string libName) : fn_starterBackend(nullptr), m_backendName(backendName)
	{
		// `.so` linux file extention for dynamic libraries
		std::string fileExtension = ".so";
#	if defined(__APPLE__)
		// `.dylib` Mac OS X file extention for dynamic libraries
		fileExtension = ".dylib";
#	endif
		// search library in working directory
		libBackend = dlopen(("./lib" + libName + fileExtension).c_str(), RTLD_LAZY);
		// fallback to binary directory
		if(!libBackend)
			libBackend = dlopen((params::inst().executablePrefix + "lib" + libName + fileExtension).c_str(), RTLD_LAZY);
		// try use LD_LIBRARY_PATH
		if(!libBackend)
			libBackend = dlopen(("lib" + libName + fileExtension).c_str(), RTLD_LAZY);
		if(!libBackend)
		{
			std::cerr << "WARNING: "<< m_backendName <<" cannot load backend library: " << dlerror() << std::endl;
			return;
		}

		// reset last error
		dlerror();
		fn_starterBackend = (starterBackend_t) dlsym(libBackend, "xmrstak_start_backend");
		const char* dlsym_error = dlerror();
		if(dlsym_error)
		{
			std::cerr << "WARNING: backend plugin " << libName << " contains no entry 'xmrstak_start_backend': " << dlsym_error << std::endl;
		}
	}

	std::vector<iBackend*>* startBackend(uint32_t threadOffset, miner_work& pWork, environment& env)
	{
		if(fn_starterBackend == nullptr)
		{
			std::vector<iBackend*>* pvThreads = new std::vector<iBackend*>();
			std::cerr << "WARNING: " << m_backendName << " Backend disabled"<< std::endl;
			return pvThreads;
		}

		return fn_starterBackend(threadOffset, pWork, env);
	}

	std::string m_backendName;

	typedef std::vector<iBackend*>* (*starterBackend_t)(uint32_t threadOffset, miner_work& pWork, environment& env);

	starterBackend_t fn_starterBackend;

	void *libBackend;
};

} // namepsace xmrstak
