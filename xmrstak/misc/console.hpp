#pragma once

#include "xmrstak/system_constants.hpp"
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <cstring>


// on MSVC sizeof(long int) = 4, gcc sizeof(long int) = 8, this is the workaround
// now we can use %llu on both compilers
inline long long unsigned int int_port(size_t i)
{
	return i;
}

enum verbosity : size_t { L0 = 0, L1 = 1, L2 = 2, L3 = 3, L4 = 4, LINF = 100};

namespace printer {

	static inline void print_msg(verbosity verbose, const char* fmt, ...)
	{
		const int verbose_level = (verbosity)system_constants::GetVerboseLevel();
		if(verbose > verbose_level)
			return;

		char buf[1024];
		size_t bpos;
		tm stime;

		time_t now = time(nullptr);
		localtime_r(&now, &stime);

		strftime(buf, sizeof(buf), "[%F %T] : ", &stime);
		bpos = strlen(buf);

		va_list args;
		va_start(args, fmt);
		vsnprintf(buf+bpos, sizeof(buf)-bpos, fmt, args);
		va_end(args);
		bpos = strlen(buf);

		if(bpos+2 >= sizeof(buf))
			return;

		buf[bpos] = '\n';
		buf[bpos+1] = '\0';

		std::cout << buf << std::endl;
	}
};

