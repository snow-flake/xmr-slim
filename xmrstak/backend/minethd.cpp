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

#include "c_cryptonight/cryptonight_aesni.hpp"
#include "console.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend//globalStates.hpp"
#include "executor.hpp"
#include "minethd.hpp"
#include "c_hwlock/do_hwlock.hpp"
#include "autoAdjust.hpp"
#include "xmrstak/system_constants.hpp"
#include "xmrstak/net/time_utils.hpp"
#include "xmrstak/net/msgstruct.hpp"
#include "xmrstak/cli/statsd.hpp"
#include "c_cryptonight/minethed_self_test.h"


#include <cmath>
#include <chrono>
#include <cstring>
#include <thread>

#if defined(__APPLE__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif //__APPLE__

namespace xmrstak
{
namespace cpu
{

bool minethd::thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id)
{
#if defined(__APPLE__)
	thread_port_t mach_thread;
	thread_affinity_policy_data_t policy = { static_cast<integer_t>(cpu_id) };
	mach_thread = pthread_mach_thread_np(h);
	return thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1) == KERN_SUCCESS;
#elif defined(__FreeBSD__)
	cpuset_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	return pthread_setaffinity_np(h, sizeof(cpuset_t), &mn) == 0;
#else
	cpu_set_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	return pthread_setaffinity_np(h, sizeof(cpu_set_t), &mn) == 0;
#endif
}

minethd::minethd(msgstruct::miner_work& pWork, size_t iNo, int iMultiway, int64_t affinity)
{
	oWork = pWork;
	bQuit = 0;
	iThreadNo = (uint8_t)iNo;
	iJobNo = 0;
	this->affinity = affinity;

	std::unique_lock<std::mutex> lck(thd_aff_set);
	std::future<void> order_guard = order_fix.get_future();

	switch (iMultiway)
	{
	case 5:
		oWorkThd = std::thread(&minethd::penta_work_main, this);
		break;
	case 4:
		oWorkThd = std::thread(&minethd::quad_work_main, this);
		break;
	case 3:
		oWorkThd = std::thread(&minethd::triple_work_main, this);
		break;
	case 2:
		oWorkThd = std::thread(&minethd::double_work_main, this);
		break;
	case 1:
	default:
		oWorkThd = std::thread(&minethd::single_work_main, this);
		break;
	}

	order_guard.wait();

	if(affinity >= 0) //-1 means no affinity
		if(!thd_setaffinity(oWorkThd.native_handle(), affinity))
			printer::print_msg(L1, "WARNING setting affinity failed.");
}

cryptonight_ctx* minethd::minethd_alloc_ctx()
{
	cryptonight_ctx* ctx;
	alloc_msg msg = { 0 };

	switch (::system_constants::GetSlowMemSetting())
	{
	case ::system_constants::never_use:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (ctx == NULL)
			printer::print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		return ctx;

	case ::system_constants::no_mlck:
		ctx = cryptonight_alloc_ctx(1, 0, &msg);
		if (ctx == NULL)
			printer::print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		return ctx;

	case ::system_constants::print_warning:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (msg.warning != NULL)
			printer::print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		if (ctx == NULL)
			ctx = cryptonight_alloc_ctx(0, 0, NULL);
		return ctx;

	case ::system_constants::always_use:
		return cryptonight_alloc_ctx(0, 0, NULL);

	case ::system_constants::unknown_value:
		return NULL; //Shut up compiler
	}

	return nullptr; //Should never happen
}

static constexpr size_t MAX_N = 5;
bool minethd::self_test()
{
	if (!minethed_self_test::test_func_selector()) {
		return false;
	}
	if (!minethed_self_test::test_func_multi_selector()) {
		return false;
	}
	return true;
}

std::vector<iBackend*> minethd::thread_starter(uint32_t threadOffset, msgstruct::miner_work& pWork)
{
	std::vector<iBackend*> pvThreads;

	//Launch the requested number of single and double threads, to distribute
	//load evenly we need to alternate single and double threads
	auto _threads = auto_threads();

	size_t i, n = _threads.processors_count;
	pvThreads.reserve(n);

	for (i = 0; i < n; i++)
	{
		auto auto_config = _threads.configs[i];

		if(auto_config.affine_to_cpu >= 0)
		{
#if defined(__APPLE__)
			printer::print_msg(L1, "WARNING on MacOS thread affinity is only advisory.");
#endif

			printer::print_msg(L1, "Starting %dx thread, affinity: %d.", auto_config.low_power_mode, (int)auto_config.affine_to_cpu);
		}
		else
			printer::print_msg(L1, "Starting %dx thread, no affinity.", auto_config.low_power_mode);
		
		minethd* thd = new minethd(pWork, i + threadOffset, auto_config.low_power_mode, auto_config.affine_to_cpu);
		pvThreads.push_back(thd);
	}

	return pvThreads;
}

void minethd::consume_work()
{
	memcpy(&oWork, &globalStates::inst().inst().oGlobalWork, sizeof(msgstruct::miner_work));
	iJobNo++;
	globalStates::inst().inst().iConsumeCnt++;
}

void minethd::single_work_main() {
	multiway_work_main<1>(cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, !CONFIG_AES_OVERRIDE, false>);
}

	void minethd::double_work_main() {
	multiway_work_main<2>(cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, !CONFIG_AES_OVERRIDE, false>);
}

void minethd::triple_work_main() {
	multiway_work_main<3>(cryptonight_triple_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, !CONFIG_AES_OVERRIDE, false>);
}

void minethd::quad_work_main() {
	multiway_work_main<4>(cryptonight_quad_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, !CONFIG_AES_OVERRIDE, false>);
}

void minethd::penta_work_main() {
	multiway_work_main<5>(cryptonight_penta_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, !CONFIG_AES_OVERRIDE, false>);
}

template<size_t N>
void minethd::prep_multiway_work(uint8_t *bWorkBlob, uint32_t **piNonce) {
	for (size_t i = 0; i < N; i++) {
		memcpy(bWorkBlob + oWork.work_blob_len * i, &oWork.work_blob_data[0], oWork.work_blob_len);
		if (i > 0) {
			piNonce[i] = (uint32_t*)(bWorkBlob + oWork.work_blob_len * i + 39);
		}
	}
}

template<size_t N>
void minethd::multiway_work_main(cn_hash_fun_multi hash_fun_multi)
{
	if(affinity >= 0) //-1 means no affinity
		do_hwlock(affinity);

	order_fix.set_value();
	std::unique_lock<std::mutex> lck(thd_aff_set);
	lck.release();
	std::this_thread::yield();

	cryptonight_ctx *ctx[MAX_N];
	uint64_t iCount = 0;
	uint64_t *piHashVal[MAX_N];
	uint32_t *piNonce[MAX_N];
	uint8_t bHashOut[MAX_N * 32];
	uint8_t bWorkBlob[sizeof(msgstruct::miner_work::work_blob_data) * MAX_N];
	uint32_t iNonce;
	msgstruct::job_result res;

	for (size_t i = 0; i < N; i++)
	{
		ctx[i] = minethd_alloc_ctx();
		piHashVal[i] = (uint64_t*)(bHashOut + 32 * i + 24);
		piNonce[i] = (i == 0) ? (uint32_t*)(bWorkBlob + 39) : nullptr;
	}

	if(!oWork.bStall)
		prep_multiway_work<N>(bWorkBlob, piNonce);

	globalStates::inst().iConsumeCnt++;

	while (bQuit == 0)
	{
		if (oWork.bStall)
		{
			/*	We are stalled here because the executor didn't find a job for us yet,
			either because of network latency, or a socket problem. Since we are
			raison d'etre of this software it us sensible to just wait until we have something*/

			while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			consume_work();
			prep_multiway_work<N>(bWorkBlob, piNonce);
			continue;
		}

		constexpr uint32_t nonce_chunk = 4096;
		int64_t nonce_ctr = 0;

		while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
		{
			if ((iCount++ & 0x7) == 0)  //Store stats every 8*N hashes
			{
				uint64_t iStamp = get_timestamp_ms();
				iHashCount.store(iCount * N, std::memory_order_relaxed);
				iTimestamp.store(iStamp, std::memory_order_relaxed);
			}

			nonce_ctr -= N;
			if(nonce_ctr <= 0)
			{
				globalStates::inst().calc_start_nonce(iNonce, nonce_chunk);
				nonce_ctr = nonce_chunk;
			}

			for (size_t i = 0; i < N; i++)
				*piNonce[i] = ++iNonce;

			hash_fun_multi(bWorkBlob, oWork.work_blob_len, bHashOut, ctx);

			for (size_t i = 0; i < N; i++)
			{
				if (*piHashVal[i] < oWork.target_data)
				{
					msgstruct_v2::result_int_t result_data;
					memcpy(&result_data[0], bHashOut + 32 * i, sizeof(msgstruct_v2::result_int_t));

					const msgstruct::job_result result(oWork.job_id_data, iNonce - N + 1 + i, result_data);
					executor::inst()->push_event_job_result(result);
				} else {
					// TODO: Log the hash was abandoned
					statsd::statsd_increment("ev.hash_abandoned");
				}
			}

			std::this_thread::yield();
		}

		consume_work();
		prep_multiway_work<N>(bWorkBlob, piNonce);
	}

	for (int i = 0; i < N; i++)
		cryptonight_free_ctx(ctx[i]);
}

} // namespace cpu
} // namepsace xmrstak
