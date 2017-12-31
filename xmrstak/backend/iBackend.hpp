#pragma once

#include "xmrstak/backend/globalStates.hpp"

#include <atomic>
#include <cstdint>
#include <climits>
#include <vector>
#include <string>

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

namespace xmrstak
{
	struct iBackend
	{

		enum BackendType : uint32_t { CPU = 1u };
		
		std::atomic<uint64_t> iHashCount;
		std::atomic<uint64_t> iTimestamp;
		uint32_t iThreadNo;

		iBackend() : iHashCount(0), iTimestamp(0)
		{
		}
	};

} // namepsace xmrstak
