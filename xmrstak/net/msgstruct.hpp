#ifndef XMR_SLIM_MSG_STRUCT_HPP
#define XMR_SLIM_MSG_STRUCT_HPP

#include <string>
#include <string.h>
#include <assert.h>
#include <cassert>
#include <array>

#include "msgstruct_v2.hpp"

// Structures that we use to pass info between threads constructors are here just to make
// the stack allocation take up less space, heap is a shared resouce that needs locks too of course

namespace msgstruct {

	struct pool_job {
		char job_id[64];
		uint8_t bWorkBlob[112];
		uint32_t iWorkLen;
		uint32_t iSavedNonce;
		std::string target;

		pool_job() : iWorkLen(0), iSavedNonce(0) {}

		pool_job(const char *job_id, const std::string &target, const uint8_t *bWorkBlob, uint32_t iWorkLen) :
				iWorkLen(iWorkLen), iSavedNonce(0), target(target) {
			assert(iWorkLen <= sizeof(pool_job::bWorkBlob));
			memcpy(this->job_id, job_id, sizeof(pool_job::job_id));
			memcpy(this->bWorkBlob, bWorkBlob, iWorkLen);
		}

		const uint64_t i_target() {
			uint64_t output = 0;
			if (target.length() <= 8) {
				uint32_t iTempInt = 0;
				char sTempStr[] = "00000000"; // Little-endian CPU FTW
				memcpy(sTempStr, target.c_str(), target.length());
				if (!hex2bin(sTempStr, 8, (unsigned char *) &iTempInt) || iTempInt == 0) {
					throw new std::runtime_error("PARSE error: Invalid target");
				}
				output = t32_to_t64(iTempInt);
			} else if (target.length() <= 16) {
				output = 0;
				char sTempStr[] = "0000000000000000";
				memcpy(sTempStr, target.c_str(), target.length());
				if (!hex2bin(sTempStr, 16, (unsigned char *) &output) || output == 0) {
					throw new std::runtime_error("PARSE error: Invalid target");
				}
			} else {
				throw new std::runtime_error("PARSE error: Job error 5");
			}
			return output;
		}

		const uint64_t i_job_diff() {
			return t64_to_diff(i_target());
		}

	private:
		inline static uint64_t t64_to_diff(uint64_t t) { return 0xFFFFFFFFFFFFFFFFULL / t; }

		inline static uint64_t t32_to_t64(uint32_t t) {
			return 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / ((uint64_t) t));
		}

		inline static unsigned char hf_hex2bin(char c, bool &err) {
			if (c >= '0' && c <= '9')
				return c - '0';
			else if (c >= 'a' && c <= 'f')
				return c - 'a' + 0xA;
			else if (c >= 'A' && c <= 'F')
				return c - 'A' + 0xA;

			err = true;
			return 0;
		}

		inline static bool hex2bin(const char *in, unsigned int len, unsigned char *out) {
			bool error = false;
			for (unsigned int i = 0; i < len; i += 2) {
				out[i / 2] = (hf_hex2bin(in[i], error) << 4) | hf_hex2bin(in[i + 1], error);
				if (error) return false;
			}
			return true;
		}

		inline static char hf_bin2hex(unsigned char c) {
			if (c <= 0x9)
				return '0' + c;
			else
				return 'a' - 0xA + c;
		}

		inline static void bin2hex(const unsigned char *in, unsigned int len, char *out) {
			for (unsigned int i = 0; i < len; i++) {
				out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
				out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
			}
		}

	};

	struct job_result {
		char job_id[64];
		uint8_t bResult[32];
		uint32_t iNonce;

		job_result() {}

		job_result(const char *job_id, uint32_t iNonce, const uint8_t *bResult) : iNonce(iNonce) {
			memcpy(this->job_id, job_id, sizeof(job_result::job_id));
			memcpy(this->bResult, bResult, sizeof(job_result::bResult));
		}

		inline const std::string job_id_str() const {
			return std::string(job_id);
		}

		inline const std::string result_str() const {
			msgstruct_v2::result_int_t tmp_result;
			memcpy(&tmp_result[0], bResult, sizeof(msgstruct_v2::result_int_t));

			auto tmp = msgstruct_v2::result_int_to_str(tmp_result);
			return std::string(&tmp[0]);
		}

		inline const std::string nonce_str() const{
			auto tmp = msgstruct_v2::nonce_int_to_str(iNonce);
			return std::string(&tmp[0]);
		}
	};

	struct sock_err {
		std::string sSocketError;
		bool silent;

		sock_err() {}

		sock_err(std::string &&err, bool silent) : sSocketError(std::move(err)), silent(silent) {}

		sock_err(sock_err &&from) : sSocketError(std::move(from.sSocketError)), silent(from.silent) {}

		sock_err &operator=(sock_err &&from) {
			assert(this != &from);
			sSocketError = std::move(from.sSocketError);
			silent = from.silent;
			return *this;
		}

		~sock_err() {}

		sock_err(sock_err const &) = delete;

		sock_err &operator=(sock_err const &) = delete;
	};

	enum ex_event_name {
		EV_INVALID_VAL, EV_SOCK_READY, EV_SOCK_ERROR,
		EV_POOL_HAVE_JOB, EV_MINER_HAVE_RESULT, EV_PERF_TICK, EV_EVAL_POOL_CHOICE,
		EV_HASHRATE_LOOP
	};

/*
   This is how I learned to stop worrying and love c++11 =).
   Ghosts of endless heap allocations have finally been exorcised. Thanks
   to the nifty magic of move semantics, string will only be allocated
   once on the heap. Considering that it makes a jorney across stack,
   heap alloced queue, to another stack before being finally processed
   I think it is kind of nifty, don't you?
   Also note that for non-arg events we only copy two qwords
*/

	struct ex_event {
		ex_event_name iName;

		union {
			pool_job oPoolJob;
			job_result oJobResult;
			sock_err oSocketError;
		};

		ex_event() { iName = EV_INVALID_VAL; }

		ex_event(std::string &&err, bool silent) : iName(EV_SOCK_ERROR), oSocketError(std::move(err), silent) {}

		ex_event(job_result dat) : iName(EV_MINER_HAVE_RESULT), oJobResult(dat) {}

		ex_event(pool_job dat) : iName(EV_POOL_HAVE_JOB), oPoolJob(dat) {}

		ex_event(ex_event_name ev) : iName(ev) {}

		// Delete the copy operators to make sure we are moving only what is needed
		ex_event(ex_event const &) = delete;

		ex_event &operator=(ex_event const &) = delete;

		ex_event(ex_event &&from) {
			iName = from.iName;
			switch (iName) {
				case EV_SOCK_ERROR:
					new(&oSocketError) sock_err(std::move(from.oSocketError));
					break;
				case EV_MINER_HAVE_RESULT:
					oJobResult = from.oJobResult;
					break;
				case EV_POOL_HAVE_JOB:
					oPoolJob = from.oPoolJob;
					break;
				default:
					break;
			}
		}

		ex_event &operator=(ex_event &&from) {
			assert(this != &from);

			if (iName == EV_SOCK_ERROR)
				oSocketError.~sock_err();

			iName = from.iName;
			switch (iName) {
				case EV_SOCK_ERROR:
					new(&oSocketError) sock_err();
					oSocketError = std::move(from.oSocketError);
					break;
				case EV_MINER_HAVE_RESULT:
					oJobResult = from.oJobResult;
					break;
				case EV_POOL_HAVE_JOB:
					oPoolJob = from.oPoolJob;
					break;
				default:
					break;
			}

			return *this;
		}

		~ex_event() {
			if (iName == EV_SOCK_ERROR)
				oSocketError.~sock_err();
		}
	};

	struct miner_work {
		char job_id[64];
		uint8_t bWorkBlob[112];
		uint32_t iWorkSize;
		uint64_t iTarget;
		bool bStall;

		miner_work() : iWorkSize(0), bStall(true) {}

		miner_work(const char *job_id, const uint8_t *bWork, uint32_t iWorkSize,
				   uint64_t iTarget) : iWorkSize(iWorkSize),
									   iTarget(iTarget), bStall(false) {
			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(this->job_id, job_id, sizeof(miner_work::job_id));
			memcpy(this->bWorkBlob, bWork, iWorkSize);
		}

		miner_work(miner_work const &) = delete;

		miner_work &operator=(miner_work const &from) {
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(job_id, from.job_id, sizeof(job_id));
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);

			return *this;
		}

		miner_work(miner_work &&from) : iWorkSize(from.iWorkSize), iTarget(from.iTarget),
										bStall(from.bStall) {
			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(job_id, from.job_id, sizeof(job_id));
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);
		}

		miner_work &operator=(miner_work &&from) {
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(bWorkBlob));
			memcpy(job_id, from.job_id, sizeof(job_id));
			memcpy(bWorkBlob, from.bWorkBlob, iWorkSize);

			return *this;
		}
	};
}

#endif // XMR_SLIM_MSG_STRUCT_HPP