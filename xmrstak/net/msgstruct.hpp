#ifndef XMR_SLIM_MSG_STRUCT_HPP
#define XMR_SLIM_MSG_STRUCT_HPP

#include <string>
#include <string.h>
#include <assert.h>

// Structures that we use to pass info between threads constructors are here just to make
// the stack allocation take up less space, heap is a shared resouce that needs locks too of course

struct pool_job
{
	char		sJobID[64];
	uint8_t		bWorkBlob[112];
	uint64_t	iTarget;
	uint32_t	iWorkLen;
	uint32_t	iSavedNonce;

	pool_job() : iWorkLen(0), iSavedNonce(0) {}
	pool_job(const char* sJobID, uint64_t iTarget, const uint8_t* bWorkBlob, uint32_t iWorkLen) :
		iTarget(iTarget), iWorkLen(iWorkLen), iSavedNonce(0)
	{
		assert(iWorkLen <= sizeof(pool_job::bWorkBlob));
		memcpy(this->sJobID, sJobID, sizeof(pool_job::sJobID));
		memcpy(this->bWorkBlob, bWorkBlob, iWorkLen);
	}
};

struct job_result
{
	uint8_t		bResult[32];
	char		sJobID[64];
	uint32_t	iNonce;
	uint32_t	iThreadId;

	job_result() {}
	job_result(const char* sJobID, uint32_t iNonce, const uint8_t* bResult, uint32_t iThreadId) : iNonce(iNonce), iThreadId(iThreadId)
	{
		memcpy(this->sJobID, sJobID, sizeof(job_result::sJobID));
		memcpy(this->bResult, bResult, sizeof(job_result::bResult));
	}
};

struct sock_err
{
	std::string sSocketError;
	bool silent;

	sock_err() {}
	sock_err(std::string&& err, bool silent) : sSocketError(std::move(err)), silent(silent) { }
	sock_err(sock_err&& from) : sSocketError(std::move(from.sSocketError)), silent(from.silent) {}

	sock_err& operator=(sock_err&& from)
	{
		assert(this != &from);
		sSocketError = std::move(from.sSocketError);
		silent = from.silent;
		return *this;
	}

	~sock_err() { }

	sock_err(sock_err const&) = delete;
	sock_err& operator=(sock_err const&) = delete;
};

enum ex_event_name { EV_INVALID_VAL, EV_SOCK_READY, EV_SOCK_ERROR,
	EV_POOL_HAVE_JOB, EV_MINER_HAVE_RESULT, EV_PERF_TICK, EV_EVAL_POOL_CHOICE, 
	EV_HASHRATE_LOOP };

/*
   This is how I learned to stop worrying and love c++11 =).
   Ghosts of endless heap allocations have finally been exorcised. Thanks
   to the nifty magic of move semantics, string will only be allocated
   once on the heap. Considering that it makes a jorney across stack,
   heap alloced queue, to another stack before being finally processed
   I think it is kind of nifty, don't you?
   Also note that for non-arg events we only copy two qwords
*/

struct ex_event
{
	ex_event_name iName;

	union
	{
		pool_job oPoolJob;
		job_result oJobResult;
		sock_err oSocketError;
	};

	ex_event() { iName = EV_INVALID_VAL;}
	ex_event(std::string&& err, bool silent) : iName(EV_SOCK_ERROR), oSocketError(std::move(err), silent) { }
	ex_event(job_result dat) : iName(EV_MINER_HAVE_RESULT), oJobResult(dat) {}
	ex_event(pool_job dat) : iName(EV_POOL_HAVE_JOB), oPoolJob(dat) {}
	ex_event(ex_event_name ev) : iName(ev) {}

	// Delete the copy operators to make sure we are moving only what is needed
	ex_event(ex_event const&) = delete;
	ex_event& operator=(ex_event const&) = delete;

	ex_event(ex_event&& from)
	{
		iName = from.iName;
		switch(iName)
		{
		case EV_SOCK_ERROR:
			new (&oSocketError) sock_err(std::move(from.oSocketError));
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

	ex_event& operator=(ex_event&& from)
	{
		assert(this != &from);

		if(iName == EV_SOCK_ERROR)
			oSocketError.~sock_err();

		iName = from.iName;
		switch(iName)
		{
		case EV_SOCK_ERROR:
			new (&oSocketError) sock_err();
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

	~ex_event()
	{
		if(iName == EV_SOCK_ERROR)
			oSocketError.~sock_err();
	}
};

#endif // XMR_SLIM_MSG_STRUCT_HPP