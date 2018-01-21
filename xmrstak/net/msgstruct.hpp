#ifndef XMR_SLIM_MSG_STRUCT_HPP
#define XMR_SLIM_MSG_STRUCT_HPP

#include <string>
#include <string.h>
#include <assert.h>
#include <cassert>

#include "msgstruct_v2.hpp"

// Structures that we use to pass info between threads constructors are here just to make
// the stack allocation take up less space, heap is a shared resouce that needs locks too of course

namespace msgstruct {

	struct pool_job {
		msgstruct_v2::job_id_str_t job_id_data;
		msgstruct_v2::work_blob_byte_t work_blob_data;
		uint32_t iWorkLen;
		uint32_t iSavedNonce;
		std::string target;

		pool_job() : iWorkLen(0), iSavedNonce(0) {}

		pool_job(const char *job_id, const std::string &target, const msgstruct_v2::work_blob_byte_t & work_blob_data, uint32_t iWorkLen) :
				iWorkLen(iWorkLen), iSavedNonce(0), target(target) {
			assert(iWorkLen <= sizeof(msgstruct_v2::work_blob_byte_t));
			assert(strlen(job_id) < sizeof(pool_job::job_id_data));

			strcpy(&job_id_data[0], job_id);
			this->work_blob_data.fill(0);
			this->work_blob_data = work_blob_data;
		}

		const uint64_t i_target() const {
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
		msgstruct_v2::job_id_str_t job_id_data;
		msgstruct_v2::result_int_t result_data;
		uint32_t iNonce;

		job_result() {}

		job_result(const msgstruct_v2::job_id_str_t & job_id_data, uint32_t iNonce, const msgstruct_v2::result_int_t & result_data) : iNonce(iNonce) {
			this->job_id_data.fill(0);
			this->job_id_data = job_id_data;

			this->result_data.fill(0);
			this->result_data = result_data;
		}

		inline const std::string job_id_str() const {
			auto const len = strlen(&job_id_data[0]);
			return std::string(&job_id_data[0], len);
		}

		inline const std::string result_str() const {
			char sResult[65];
			msgstruct_v2::utils::bin2hex(&result_data[0], 32, sResult);
			sResult[64] = '\0';
			return std::string(sResult);

		}

		inline const std::string nonce_str() const {
			char sNonce[9];
			msgstruct_v2::utils::bin2hex((unsigned char *) &iNonce, 4, sNonce);
			sNonce[8] = '\0';
			return std::string(sNonce);
		}
	};

	struct sock_err {
		std::string sSocketError;
		bool silent;

		sock_err() {}

		sock_err(const std::string &err, bool silent) : sSocketError(err), silent(silent) {}

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

	struct ex_event {
		ex_event_name iName;

		union {
			pool_job oPoolJob;
			job_result oJobResult;
			sock_err oSocketError;
		};

		ex_event() { iName = EV_INVALID_VAL; }

		ex_event(const std::string & err, bool silent) : iName(EV_SOCK_ERROR), oSocketError(err, silent) {}

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
		msgstruct_v2::job_id_str_t job_id_data;
		msgstruct_v2::work_blob_byte_t work_blob_data;
		uint32_t iWorkSize;
		uint64_t iTarget;
		bool bStall;

		miner_work() : iWorkSize(0), bStall(true) {}

		miner_work(const msgstruct_v2::job_id_str_t & job_id_data, const msgstruct_v2::work_blob_byte_t & work_blob_data, uint32_t iWorkSize,
				   uint64_t iTarget) : iWorkSize(iWorkSize), iTarget(iTarget), bStall(false) {
			this->job_id_data = job_id_data;

			assert(iWorkSize <= sizeof(msgstruct_v2::work_blob_byte_t));
			this->work_blob_data.fill(0);
			this->work_blob_data = work_blob_data;
		}

		miner_work(miner_work const &) = delete;

		miner_work &operator=(miner_work const &from) {
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;

			return *this;
		}

		miner_work(miner_work &&from) : iWorkSize(from.iWorkSize), iTarget(from.iTarget),
										bStall(from.bStall) {
			assert(iWorkSize <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;
		}

		miner_work &operator=(miner_work &&from) {
			assert(this != &from);

			iWorkSize = from.iWorkSize;
			iTarget = from.iTarget;
			bStall = from.bStall;

			assert(iWorkSize <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;

			return *this;
		}
	};
}

#endif // XMR_SLIM_MSG_STRUCT_HPP