//
// Created by Snow Flake on 1/24/18.
//

#include "do_cryptonight.h"
#include "cryptonight.hpp"
#include <string.h>
#include <array>


typedef std::array<uint8_t, 112> work_blob_byte_t;
typedef std::array<uint8_t, 32> result_int_t;

static constexpr size_t MAX_N = 5;
typedef std::array<uint8_t, 32 * MAX_N> hashing_blob_byte_t;
typedef std::array<uint8_t, 112 * MAX_N> work_blob_batch_byte_t;

typedef std::array<uint8_t, 32> result_int_t;

typedef std::array<uint64_t *, MAX_N> hash_val_value_t;
typedef std::array<uint32_t, MAX_N> nonce_value_t;

typedef std::array<std::shared_ptr<cryptonight_ctx_cls>, MAX_N> cryptonight_ctx_cls_array_t;

inline void calc_start_nonce(std::atomic<uint32_t> & iGlobalNonce, uint32_t& nonce, uint32_t reserve_count)
{
	nonce = iGlobalNonce.fetch_add(reserve_count);
}


void multiway_work_main(std::atomic<uint32_t> & iGlobalNonce, const work_blob_byte_t & work_blob_data, const size_t work_blob_len, size_t thread_id)
{
	// TODO: do_hwlock(thread_id);
	order_fix.set_value();

	cryptonight_ctx_cls_array_t ctx;
	for(int i = 0; i < MAX_N; i++) {
		ctx[i] = std::shared_ptr<cryptonight_ctx_cls>(new cryptonight_ctx_cls());
	}

	uint64_t iCount = 0;
	hash_val_value_t piHashVal; piHashVal.fill(0);
	nonce_value_t piNonce; piNonce.fill(0);
	hashing_blob_byte_t bHashOut; bHashOut.fill(0);
	work_blob_batch_byte_t bWorkBlob; bWorkBlob.fill(0);

	uint32_t iNonce;
	result_int_t result;

	for (size_t i = 0; i < CONFIG_SYSTEM_NPROC; i++) {
		piHashVal[i] = (uint64_t*)(&bHashOut[0] + 32 * i + 24);
		piNonce[i] = (i == 0) ? (uint32_t*)(&bWorkBlob[0] + 39) : nullptr;
	}

	for (size_t i = 0; i < CONFIG_SYSTEM_NPROC; i++) {
		memcpy(&bWorkBlob[0] + work_blob_len * i, &work_blob_data[0], work_blob_len);
		if (i > 0) {
			piNonce[i] = (uint32_t*)(&bWorkBlob[0] + work_blob_len * i + 39);
		}
	}

	constexpr uint32_t nonce_chunk = 4096;
	int64_t nonce_ctr = 0;

	while (true) {
		nonce_ctr -= CONFIG_SYSTEM_NPROC;
		if(nonce_ctr <= 0) {
			calc_start_nonce(iGlobalNonce, iNonce, nonce_chunk);
			nonce_ctr = nonce_chunk;
		}

		for (size_t i = 0; i < CONFIG_SYSTEM_NPROC; i++){
			*piNonce[i] = ++iNonce;
		}

		hash_fun_multi(bWorkBlob, oWork.work_blob_len, bHashOut, ctx);
		cryptonight_double_hash<MONERO_MASK, MONERO_ITER, MONERO_MEMORY, false, false>

		for (size_t i = 0; i < CONFIG_SYSTEM_NPROC; i++)
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

	for (int i = 0; i < CONFIG_SYSTEM_NPROC; i++) {
		cryptonight_free_ctx(ctx[i]);
	}
}
