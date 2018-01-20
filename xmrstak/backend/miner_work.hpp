#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstring>

namespace xmrstak
{
	struct miner_work
	{
		std::string job_id;  // const char job_id[64];
		uint8_t     bWorkBlob[112];
		uint32_t    iWorkSize;
		uint64_t    iTarget;
		bool        bStall;

		miner_work() : iWorkSize(0), bStall(true) { }

		miner_work(const std::string & job_id, const uint8_t* bWork, uint32_t iWorkSize,
			uint64_t iTarget) : iWorkSize(iWorkSize),
			iTarget(iTarget), bStall(false), job_id(job_id)
		{
			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(this->bWorkBlob, bWork, iWorkSize);
		}

		miner_work(miner_work const&) = delete;

		miner_work& operator=(miner_work const& from)
		{
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(job_id, from.job_id, sizeof(job_id));
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);

			return *this;
		}

		miner_work(miner_work&& from) : iWorkSize(from.iWorkSize), iTarget(from.iTarget),
			bStall(from.bStall), job_id(from.job_id)
		{
			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);
		}

		miner_work& operator=(miner_work&& from)
		{
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(bWorkBlob));
			job_id = from.job_id;
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);

			return *this;
		}
	};
} // namepsace xmrstak
