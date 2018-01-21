#ifndef XMR_SLIM_MSG_STRUCT_V2_HPP
#define XMR_SLIM_MSG_STRUCT_V2_HPP

#include <string>
#include <string.h>
#include <assert.h>
#include <cassert>
#include <array>

namespace msgstruct_v2 {

	namespace utils {
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

	}

	typedef std::array<char, 64> job_id_str_t;
	typedef std::array<char, 112*1> work_blob_str_t;
	typedef std::array<uint8_t, 112> work_blob_byte_t;

	typedef std::array<char, 9> nonce_str_t;
	typedef uint32_t nonce_int_t;

	typedef std::array<char, 32*1+1> result_str_t;
	typedef std::array<uint8_t, 32> result_int_t;


	inline static nonce_str_t nonce_int_to_str(const nonce_int_t nonce) {
		nonce_str_t sNonce;
		msgstruct_v2::utils::bin2hex((unsigned char *) &nonce, 4, &sNonce[0]);
		sNonce[8] = '\0';
		return sNonce;
	}

	inline static result_str_t result_int_to_str(const result_int_t & result) {
		result_str_t sResult;
		msgstruct_v2::utils::bin2hex((const unsigned char *)&result[0], 32, &sResult[0]);
		sResult[64] = '\0';
		return sResult;
	}

	inline uint64_t target_str_to_int(const std::string & target) {
		uint64_t i_target = 0;
		if (target.length() <= 8) {
			uint32_t iTempInt = 0;
			char sTempStr[] = "00000000"; // Little-endian CPU FTW
			memcpy(sTempStr, target.c_str(), target.length());
			if (!msgstruct_v2::utils::hex2bin(sTempStr, 8, (unsigned char *) &iTempInt) || iTempInt == 0) {
				throw new std::runtime_error("PARSE error: Invalid target");
			}
			i_target = msgstruct_v2::utils::t32_to_t64(iTempInt);
		} else if (target.length() <= 16) {
			i_target = 0;
			char sTempStr[] = "0000000000000000";
			memcpy(sTempStr, target.c_str(), target.length());
			if (!msgstruct_v2::utils::hex2bin(sTempStr, 16, (unsigned char *) &i_target) || i_target == 0) {
				throw new std::runtime_error("PARSE error: Invalid target");
			}
		} else {
			throw new std::runtime_error("PARSE error: Job error 5");
		}
		return i_target;
	}

	inline work_blob_byte_t blob_str_to_bytes(const std::string & blob) {
		assert(blob.length() / 2 <= sizeof(work_blob_byte_t));
		assert(blob.length() <= sizeof(work_blob_str_t));

		work_blob_str_t blob_str;
		blob_str.fill(0);
		std::copy(blob.begin(), blob.end(), blob_str.data());

		work_blob_byte_t blob_byte;
		blob_byte.fill(0);
		if (!msgstruct_v2::utils::hex2bin(&blob_str[0], blob.length(), &blob_byte[0])) {
			throw new std::runtime_error("PARSE error: Job error 4");
		}

		return blob_byte;
	}

	struct pool_job {
		const std::string job_id, blob, target;
		const uint64_t i_target;
		const uint32_t i_saved_nonce;

		pool_job(const std::string & job_id, const std::string & target, const std::string & blob, const uint32_t i_saved_nonce = 0) :
				job_id(job_id),
				target(target),
				blob(blob),
				i_target(target_str_to_int(target)),
				i_saved_nonce(i_saved_nonce)
		{}

		const uint64_t i_job_diff() const {
			return msgstruct_v2::utils::t64_to_diff(this->i_target);
		}

	};


	struct job_result {
		const std::string job_id, blob, target, result, nonce;
		job_result(const std::string & job_id, const std::string & target, const std::string & blob, const std::string & result, const std::string & nonce):
				job_id(job_id),
				target(target),
				blob(blob),
				result(result),
				nonce(nonce)
		{}
	};

	enum ex_event_name {
		EV_INVALID_VAL, EV_SOCK_READY, EV_SOCK_ERROR,
		EV_POOL_HAVE_JOB, EV_MINER_HAVE_RESULT, EV_PERF_TICK, EV_EVAL_POOL_CHOICE,
		EV_HASHRATE_LOOP
	};


	struct ex_event {
		ex_event_name iName;
		std::string sSocketError;
		std::shared_ptr<pool_job> pool_job_ptr;
		std::shared_ptr<job_result> job_result_ptr;

		ex_event() {
			iName = msgstruct_v2::EV_INVALID_VAL;
		}
	};

	struct miner_work {
		job_id_str_t job_id_byte;
		work_blob_byte_t blob_byte;
		uint32_t job_id_len;
		uint32_t blob_len;
		uint64_t i_target;
		bool bStall;

		miner_work() : i_target(0), blob_len(0), bStall(true) {
			job_id_byte.fill(0);
			blob_byte.fill(0);
		}

		miner_work(
				const std::string & job_id,
				const std::string & blob,
				const uint64_t _i_target,
				const bool _b_stall
		) {
			job_id_byte.fill(0);
			blob_byte.fill(0);

			assert(blob_len <= sizeof(blob_byte));
			assert(job_id_len <= sizeof(job_id_byte));

			std::copy(job_id.begin(), job_id.end(), job_id_byte.data());
			blob_byte = blob_str_to_bytes(blob);
			blob_len = blob.length();
			i_target = _i_target;
			bStall = _b_stall;
		}

		miner_work(
			const job_id_str_t & _job_id,
			const work_blob_byte_t & _b_work_blob,
			const uint32_t i_work_size,
			const uint64_t _i_target,
			const bool _b_stall
		) {
			job_id_byte.fill(0);
			blob_byte.fill(0);

			assert(blob_len <= sizeof(blob_byte));
			job_id_byte = _job_id;
			blob_byte = _b_work_blob;
			blob_len = i_work_size;
			i_target = _i_target;
			bStall = _b_stall;
		}

		miner_work(miner_work &&from) {
			this->job_id_byte.fill(0);
			this->blob_byte.fill(0);

			assert(blob_len <= sizeof(blob_byte));
			this->job_id_byte = from.job_id_byte ;
			this->blob_byte = from.blob_byte;
			this->blob_len = from.blob_len;
			this->i_target = from.i_target;
			this->bStall = from.bStall;
		}

		miner_work(miner_work const &) = delete;

		miner_work &operator=(miner_work const &from) noexcept {
			this->job_id_byte = from.job_id_byte ;
			this->blob_byte = from.blob_byte;
			this->blob_len = from.blob_len;
			this->i_target = from.i_target;
			this->bStall = from.bStall;
			return *this;
		}

		miner_work &operator=(miner_work &&from) {
			assert(this != &from);
			assert(blob_len <= sizeof(blob_byte));

			this->job_id_byte = from.job_id_byte ;
			this->blob_byte = from.blob_byte;
			this->blob_len = from.blob_len;
			this->i_target = from.i_target;
			this->bStall = from.bStall;

			return *this;
		}
	};
}

#endif // XMR_SLIM_MSG_STRUCT_V2_HPP