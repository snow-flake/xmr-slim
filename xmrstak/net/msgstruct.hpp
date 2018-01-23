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
	private:
		msgstruct_v2::job_id_str_t job_id_data;
		msgstruct_v2::work_blob_byte_t work_blob_data;
		uint32_t job_id_len, work_blob_len;
		uint32_t iSavedNonce;
		std::string target;

	public:
		pool_job() : work_blob_len(0), iSavedNonce(0), job_id_len(0) {}

		const msgstruct_v2::job_id_str_t & get_job_id_data() const { return job_id_data; }
		void set_job_id(const std::string & input) {
			job_id_data.fill(0);
			strcpy(&job_id_data[0], input.c_str());
		}

		const msgstruct_v2::work_blob_byte_t & get_work_blob_data() const { return work_blob_data; }
		const uint32_t get_work_blob_len() const { return work_blob_len; };
		bool set_blob(const std::string & input) {
			if (!msgstruct_v2::utils::hex2bin(input.c_str(), input.length(), &work_blob_data[0])) {
				return false;
			}

			work_blob_len = input.length() / 2;
			return true;
		}

		const std::string & get_target() const { return target; }
		void set_target(const std::string & input) { target = input; }

		const uint32_t get_iSavedNonce() const { return iSavedNonce; }

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
	private:
		msgstruct_v2::job_id_str_t job_id_data;
		msgstruct_v2::result_int_t result_data;
		uint32_t iNonce;
	public:
		const msgstruct_v2::job_id_str_t & get_job_id_data() const { return job_id_data; };
		void set_job_id_data(const msgstruct_v2::job_id_str_t & input) { job_id_data = input; };

		const msgstruct_v2::result_int_t & get_result_data() const { return result_data; };
		void set_result_data(const msgstruct_v2::result_int_t & input) { result_data = input; };

		const uint32_t get_iNonce() const { return iNonce; };
		void set_iNonce(const uint32_t input) { iNonce = input; };

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
		uint32_t work_blob_len;
		uint64_t target_data;
		bool bStall;

		miner_work() : work_blob_len(0), bStall(true) {}

		miner_work(const msgstruct_v2::job_id_str_t & job_id_data, const msgstruct_v2::work_blob_byte_t & work_blob_data, uint32_t work_blob_len,
				   uint64_t target_data) : work_blob_len(work_blob_len), target_data(target_data), bStall(false) {
			this->job_id_data = job_id_data;

			assert(work_blob_len <= sizeof(msgstruct_v2::work_blob_byte_t));
			this->work_blob_data.fill(0);
			this->work_blob_data = work_blob_data;
		}

		miner_work(miner_work const &) = delete;

		miner_work &operator=(miner_work const &from) {
			assert(this != &from);

			work_blob_len = from.work_blob_len;
			target_data = from.target_data;
			bStall = from.bStall;

			assert(work_blob_len <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;

			return *this;
		}

		miner_work(miner_work &&from) : work_blob_len(from.work_blob_len), target_data(from.target_data),
										bStall(from.bStall) {
			assert(work_blob_len <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;
		}

		miner_work &operator=(miner_work &&from) {
			assert(this != &from);

			work_blob_len = from.work_blob_len;
			target_data = from.target_data;
			bStall = from.bStall;

			assert(work_blob_len <= sizeof(msgstruct_v2::work_blob_byte_t));
			job_id_data = from.job_id_data;
			work_blob_data = from.work_blob_data;

			return *this;
		}
	};
}

#endif // XMR_SLIM_MSG_STRUCT_HPP