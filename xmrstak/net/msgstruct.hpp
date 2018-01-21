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

		const uint64_t i_target() const {
			return msgstruct_v2::target_str_to_int(target);
		}

		const uint64_t i_job_diff() const {
			return msgstruct_v2::utils::t64_to_diff(i_target());
		}
	};

	typedef std::shared_ptr<const pool_job> pool_job_const_ptr_t;

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

	typedef std::shared_ptr<const job_result> job_result_const_ptr_t;

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
	typedef std::shared_ptr<const sock_err> sock_err_const_ptr_t;


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
	public:
		ex_event_name iName;
		pool_job_const_ptr_t pool_job_const_ptr;
		job_result_const_ptr_t job_result_const_ptr;
		sock_err_const_ptr_t sock_err_const_ptr;

		static std::shared_ptr<const ex_event> make_name_event(ex_event_name name) {
			std::shared_ptr<ex_event> event(new ex_event());
			event->iName = name;
			return event;
		}

		static std::shared_ptr<const ex_event> make_pool_job(pool_job_const_ptr_t pool_job_const_ptr) {
			std::shared_ptr<ex_event> event(new ex_event());
			event->iName = EV_POOL_HAVE_JOB;
			event->pool_job_const_ptr = pool_job_const_ptr;
			return event;
		}

		static std::shared_ptr<const ex_event> make_pool_result(job_result_const_ptr_t job_result_const_ptr) {
			std::shared_ptr<ex_event> event(new ex_event());
			event->iName = EV_MINER_HAVE_RESULT;
			event->job_result_const_ptr= job_result_const_ptr;
			return event;
		}

		static std::shared_ptr<const ex_event> make_error(sock_err_const_ptr_t sock_err_const_ptr) {
			std::shared_ptr<ex_event> event(new ex_event());
			event->iName = EV_SOCK_ERROR;
			event->sock_err_const_ptr = sock_err_const_ptr;
			return event;
		}

	private:
		ex_event()
		{}
	};

	typedef std::shared_ptr<const ex_event> ex_event_const_ptr_t;

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