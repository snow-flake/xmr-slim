#pragma once

#include "environment.hpp"
#include "console.hpp"
#include "xmrstak/net/msgstruct.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <cstring>

namespace xmrstak
{

struct pool_data
{
	uint32_t iSavedNonce;
	pool_data() : iSavedNonce(0)
	{}
};

struct globalStates
{
	static inline globalStates& inst()
	{
		auto& env = environment::inst();
		if(env.pglobalStates == nullptr)
			env.pglobalStates = new globalStates;
		return *env.pglobalStates;
	}

	//pool_data is in-out winapi style
	void switch_work(msgstruct::miner_work& pWork, pool_data& dat);

	inline void calc_start_nonce(uint32_t& nonce, uint32_t reserve_count)
	{
		nonce = iGlobalNonce.fetch_add(reserve_count);
	}

	msgstruct::miner_work oGlobalWork;
	std::atomic<uint64_t> iGlobalJobNo;
	std::atomic<uint64_t> iConsumeCnt;
	std::atomic<uint32_t> iGlobalNonce;
	uint64_t iThreadCount;

private:
	globalStates() : iThreadCount(0)
	{
	}
};

} // namepsace xmrstak
