//
// Created by Snow Flake on 1/15/18.
//

#ifndef XMR_STAK_TIME_UTILS_H
#define XMR_STAK_TIME_UTILS_H

#include <chrono>

//Get steady_clock timestamp - misc helper function
inline unsigned long get_timestamp()
{
	using namespace std::chrono;
	return time_point_cast<duration<long long int>>(steady_clock::now()).time_since_epoch().count();
}

//Get milisecond timestamp
inline unsigned long get_timestamp_ms()
{
	using namespace std::chrono;
	if (high_resolution_clock::is_steady) {
		return time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
	} else {
		return time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();
	}
}

#endif //XMR_STAK_TIME_UTILS_H
