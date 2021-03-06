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

#include "executor.hpp"
#include "xmrstak/net/jpsock.hpp"

#include "telemetry.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/backend/minethd.hpp"
#include "console.hpp"
#include "xmrstak/system_constants.hpp"
#include "xmrstak/net/time_utils.hpp"
#include "xmrstak/cli/statsd.hpp"
#include "xmrstak/net/msgstruct.hpp"

#include <thread>
#include <cmath>
#include <algorithm>
#include <functional>
#include <assert.h>


std::vector<xmrstak::iBackend*>* thread_starter(msgstruct::miner_work& pWork)
{
	xmrstak::globalStates::inst().iGlobalJobNo = 0;
	xmrstak::globalStates::inst().iConsumeCnt = 0;
	std::vector<xmrstak::iBackend*>* pvThreads = new std::vector<xmrstak::iBackend*>;

	auto cpuThreads = xmrstak::cpu::minethd::thread_starter(static_cast<uint32_t>(pvThreads->size()), pWork);
	pvThreads->insert(std::end(*pvThreads), std::begin(cpuThreads), std::end(cpuThreads));
	if(cpuThreads.size() == 0) {
		throw new std::runtime_error("Error: Backend CPU disabled.");
	}

	xmrstak::globalStates::inst().iThreadCount = pvThreads->size();
	return pvThreads;
}


executor::executor()
{
}

void executor::push_timed_event(msgstruct::ex_event_name name, size_t sec)
{
	std::shared_ptr<const msgstruct::ex_event> ptr = std::shared_ptr<const msgstruct::ex_event>(
			new msgstruct::ex_event(name)
	);

	std::unique_lock<std::mutex> lck(timed_event_mutex);
	lTimedEvents.emplace_back(ptr, sec_to_ticks(sec));
}

void executor::ex_clock_thd()
{
	size_t tick = 0;
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(size_t(iTickTime)));

		push_event_name(msgstruct::EV_PERF_TICK);

		//Eval pool choice every fourth tick
		if((tick++ & 0x03) == 0)
			push_event_name(msgstruct::EV_EVAL_POOL_CHOICE);

		// Service timed events
		std::unique_lock<std::mutex> lck(timed_event_mutex);
		std::list<timed_event>::iterator ev = lTimedEvents.begin();
		while (ev != lTimedEvents.end())
		{
			ev->ticks_left--;
			if(ev->ticks_left == 0)
			{
				push_event(ev->event_ptr);
				ev = lTimedEvents.erase(ev);
			}
			else
				ev++;
		}
		lck.unlock();
	}
}

bool executor::is_pool_live() {
	if (pool_ptr.get() == nullptr) {
		return false;
	}

	size_t limit = system_constants::GetGiveUpLimit();
	size_t wait = system_constants::GetNetRetry();

	if(limit == 0 || false) limit = (-1); //No limit = limit of 2^64-1

	// Only eval live pools
	size_t num, dtime;
	if(pool_ptr->get_disconnects(num, dtime))
		set_timestamp();
	if (dtime == 0 || (dtime >= wait && num <= limit)) {
		return true;
	}
	if(num > limit) {
		printer::print_msg(L0, "All pools are over give up limit. Exitting.");
		exit(0);
	}

	return false;
}

/*
 * This event is called by the timer and whenever something relevant happens.
 * The job here is to decide if we want to connect, disconnect, or switch jobs (or do nothing)
 */
void executor::eval_pool_choice()
{
	if (is_pool_live() && pool_ptr->is_running() && pool_ptr->is_logged_in()) {
		return;
	}

	// Special case - if we are without a pool, connect to all find a live pool asap
	if(pool_ptr.get() == nullptr) {
		std::shared_ptr<jpsock> pool = std::shared_ptr<jpsock>(new jpsock());
		if(pool->can_connect()) {
			auto pool_address = system_constants::get_pool_pool_address();
			printer::print_msg(L1, "Fast-connecting to %s pool ...", pool_address.c_str());
			std::string error;
			if(!pool->connect(error))
				log_socket_error(std::move(error));
			else
				pool_ptr = pool;
		}
		return;
	}

	std::shared_ptr<jpsock> goal = std::shared_ptr<jpsock>(new jpsock());
	if(!goal->is_running() && goal->can_connect())
	{
		auto pool_address = system_constants::get_pool_pool_address();
		printer::print_msg(L1, "Connecting to %s pool ...", pool_address.c_str());

		std::string error;
		if(!goal->connect(error))
			log_socket_error(std::move(error));
		else
			pool_ptr = goal;
		return;
	}

	if(goal->is_logged_in())
	{
		msgstruct::pool_job oPoolJob;
		if(!goal->get_current_job(oPoolJob)) {
			goal->disconnect();
			return;
		}
		pool_ptr = goal;

		on_pool_have_job(oPoolJob);
		reset_stats();
		return;
	}
}

void executor::log_socket_error(std::string sError)
{
	std::string pool_name;
	pool_name.reserve(128);
	pool_name.append("[").append(system_constants::get_pool_pool_address()).append("] ");
	sError.insert(0, pool_name);

	vSocketLog.emplace_back(std::move(sError));
	printer::print_msg(L1, "SOCKET ERROR - %s", vSocketLog.back().msg.c_str());

	push_event_name(msgstruct::EV_EVAL_POOL_CHOICE);
}

void executor::log_result_error(std::string sError)
{
	size_t i = 1, ln = vMineResults.size();
	for(; i < ln; i++)
	{
		if(vMineResults[i].compare(sError))
		{
			vMineResults[i].increment();
			break;
		}
	}

	if(i == ln) //Not found
		vMineResults.emplace_back(std::move(sError));
	else
		sError.clear();
}

void executor::log_result_ok(uint64_t iActualDiff)
{
	iPoolHashes += iPoolDiff;

	size_t ln = iTopDiff.size() - 1;
	if(iActualDiff > iTopDiff[ln])
	{
		iTopDiff[ln] = iActualDiff;
		std::sort(iTopDiff.rbegin(), iTopDiff.rend());
	}

	vMineResults[0].increment();
}


void executor::on_sock_ready()
{
	jpsock* pool = pool_ptr.get();
	auto pool_address = system_constants::get_pool_pool_address();
	printer::print_msg(L1, "Pool %s connected. Logging in...", pool_address.c_str());

	if(!pool->cmd_login())
	{
		if(!pool->have_sock_error())
		{
			log_socket_error(pool->get_call_error());
			pool->disconnect();
		}
	}
}

void executor::on_sock_error(const msgstruct::sock_err &err) {
	if (pool_ptr.get()) {
		pool_ptr.get()->disconnect();
	}

	pool_ptr = std::shared_ptr<jpsock>();
	if(!err.silent) {
		std::string tmp_error = err.sSocketError;
		log_socket_error(tmp_error);
	}
}

void executor::on_pool_have_job(const msgstruct::pool_job& oPoolJob) {
	jpsock* pool = pool_ptr.get();
	if(pool == nullptr) {
		return;
	}

	msgstruct::miner_work oWork(
			oPoolJob.get_job_id_data(),
			oPoolJob.get_work_blob_data(),
			oPoolJob.get_work_blob_len(),
			oPoolJob.i_target()
	);

	xmrstak::pool_data dat;
	dat.iSavedNonce = oPoolJob.get_iSavedNonce();

	xmrstak::globalStates::inst().switch_work(oWork, dat);

	if(iPoolDiff != pool->get_current_diff()) {
		iPoolDiff = pool->get_current_diff();
		printer::print_msg(L2, "Difficulty changed. Now: %llu.", int_port(iPoolDiff));
	}

	printer::print_msg(L3, "New block detected.");
}

void executor::on_miner_result(const msgstruct::job_result& oResult) {
	const std::string job_id = oResult.job_id_str();
	const std::string nonce = oResult.nonce_str();
	const std::string result = oResult.result_str();

	std::cout << __FILE__ << ":" << __LINE__ << ":executor::on_miner_result: Miner result: "
			  << "nonce=" << nonce << ", "
			  << "job_id=" << job_id << ", "
			  << "result=" << result << std::endl;

	jpsock* pool = pool_ptr.get();
	if(pool == nullptr) {
		std::cout << __FILE__ << ":" << __LINE__ << ":executor::on_miner_result: Ignoring miner result, pool is disconnected" << std::endl;
		return;
	}

	if (!pool->is_running() || !pool->is_logged_in()) {
		log_result_error("[NETWORK ERROR]");
		return;
	}

	size_t t_start = get_timestamp_ms();
	bool bResult = pool->cmd_submit(job_id, nonce, result);
	size_t t_len = get_timestamp_ms() - t_start;

	if(t_len > 0xFFFF) {
		t_len = 0xFFFF;
	}
	iPoolCallTimes.push_back((uint16_t)t_len);

	if(bResult) {
		uint64_t* targets = (uint64_t*)&oResult.result_data[0];
		log_result_ok(jpsock::t64_to_diff(targets[3]));
		printer::print_msg(L3, "Result accepted by the pool.");
	} else {
		if(!pool->have_sock_error()) {
			printer::print_msg(L3, "Result rejected by the pool.");

			std::string error = pool->get_call_error();
			if(strncasecmp(error.c_str(), "Unauthenticated", 15) == 0) {
				printer::print_msg(L2, "Your miner was unable to find a share in time. Either the pool difficulty is too high, or the pool timeout is too low.");
				pool->disconnect();
			}
			log_result_error(error);
		}
		else {
			log_result_error("[NETWORK ERROR]");
		}
	}
}

#include <signal.h>
void disable_sigpipe()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, 0) == -1)
		printer::print_msg(L1, "ERROR: Call to sigaction failed!");
}

void executor::ex_main()
{
	disable_sigpipe();

	assert(1000 % iTickTime == 0);

	msgstruct::miner_work oWork;

	// \todo collect all backend threads
	pvThreads = thread_starter(oWork);

	if(pvThreads->size()==0)
	{
		printer::print_msg(L1, "ERROR: No miner backend enabled.");
		std::exit(1);
	}

	telem = new xmrstak::telemetry(pvThreads->size());

	set_timestamp();
	pool_ptr = std::shared_ptr<jpsock>(new jpsock());

	std::thread clock_thd(&executor::ex_clock_thd, this);

	eval_pool_choice();

	// Place the default success result at position 0, it needs to
	// be here even if our first result is a failure
	vMineResults.emplace_back();

	// If the user requested it, start the autohash printer
	if(system_constants::GetVerboseLevel() >= 4) {
		push_timed_event(msgstruct::EV_HASHRATE_LOOP, system_constants::GetAutohashTime());
	}

	size_t cnt = 0;
	while (true)
	{
		std::shared_ptr<const msgstruct::ex_event> ev = oEventQ.pop();
		switch (ev->iName)
		{
		case msgstruct::EV_SOCK_READY:
			statsd::statsd_increment("ev.sock_ready");
			on_sock_ready();
			break;

		case msgstruct::EV_SOCK_ERROR:
			statsd::statsd_increment("ev.sock_error");
			on_sock_error(ev->oSocketError);
			break;

		case msgstruct::EV_POOL_HAVE_JOB:
			statsd::statsd_increment("ev.pool_have_job");
			on_pool_have_job(ev->oPoolJob);
			break;

		case msgstruct::EV_MINER_HAVE_RESULT:
			statsd::statsd_increment("ev.miner_have_result");
			on_miner_result(ev->oJobResult);
			break;

		case msgstruct::EV_EVAL_POOL_CHOICE:
			statsd::statsd_increment("ev.eval_pool_choice");
			eval_pool_choice();
			break;


		case msgstruct::EV_PERF_TICK:
			statsd::statsd_increment("ev.perf_tick");
			for (int i = 0; i < pvThreads->size(); i++) {
				uint64_t iHashCount = pvThreads->at(i)->iHashCount.load(std::memory_order_relaxed);
				uint64_t iTimestamp = pvThreads->at(i)->iTimestamp.load(std::memory_order_relaxed);
				telem->push_perf_value(i, iHashCount, iTimestamp);
				statsd::statsd_gauge("i_hash_count", iHashCount);
			}

			if((cnt++ & 0xF) == 0) //Every 16 ticks
			{
				double fHps = 0.0;
				double fTelem;
				bool normal = true;

				for (int i = 0; i < pvThreads->size(); i++)
				{
					fTelem = telem->calc_telemetry_data(10000, i);
					if(std::isnormal(fTelem))
					{
						fHps += fTelem;

					}
					else
					{
						normal = false;
						break;
					}
				}

				if(normal && fHighestHps < fHps)
					fHighestHps = fHps;

				statsd::statsd_gauge("f_hps", fHps);
				statsd::statsd_gauge("max_f_hps", fHighestHps);
			}
			break;

		case msgstruct::EV_HASHRATE_LOOP:
			print_report();
			push_timed_event(msgstruct::EV_HASHRATE_LOOP, system_constants::GetAutohashTime());
			break;

		case msgstruct::EV_INVALID_VAL:
		default:
			assert(false);
			break;
		}
	}
}

inline const char* hps_format(double h, char* buf, size_t l)
{
	if(std::isnormal(h) || h == 0.0)
	{
		snprintf(buf, l, " %6.1f", h);
		return buf;
	}
	else
		return "   (na)";
}

std::string executor::hashrate_report()
{
	std::string out;
	out.reserve(2048 + pvThreads->size() * 64);

	char num[32];
	double fTotal[3] = { 0.0, 0.0, 0.0};

	{
		std::vector<xmrstak::iBackend*> backEnds;
		std::copy_if(pvThreads->begin(), pvThreads->end(), std::back_inserter(backEnds), [&](xmrstak::iBackend* backend) { return true; });

		size_t nthd = backEnds.size();
		if(nthd != 0)
		{
			size_t i;
			std::string name("cpu");
			std::transform(name.begin(), name.end(), name.begin(), ::toupper);
			
			out.append("HASHRATE REPORT - ").append(name).append("\n");
			out.append("| ID |    10s |    60s |    15m |");
			if(nthd != 1)
				out.append(" ID |    10s |    60s |    15m |\n");
			else
				out.append(1, '\n');

			for (i = 0; i < nthd; i++)
			{
				double fHps[3];

				uint32_t tid = backEnds[i]->iThreadNo;
				fHps[0] = telem->calc_telemetry_data(10000, tid);
				fHps[1] = telem->calc_telemetry_data(60000, tid);
				fHps[2] = telem->calc_telemetry_data(900000, tid);

				snprintf(num, sizeof(num), "| %2u |", (unsigned int)i);
				out.append(num);
				out.append(hps_format(fHps[0], num, sizeof(num))).append(" |");
				out.append(hps_format(fHps[1], num, sizeof(num))).append(" |");
				out.append(hps_format(fHps[2], num, sizeof(num))).append(1, ' ');

				fTotal[0] += fHps[0];
				fTotal[1] += fHps[1];
				fTotal[2] += fHps[2];

				if((i & 0x1) == 1) //Odd i's
					out.append("|\n");
			}

			if((i & 0x1) == 1) //We had odd number of threads
				out.append("|\n");

			if(nthd != 1)
				out.append("-----------------------------------------------------\n");
			else
				out.append("---------------------------\n");
		}
	}

	out.append("Totals:  ");
	out.append(hps_format(fTotal[0], num, sizeof(num)));
	out.append(hps_format(fTotal[1], num, sizeof(num)));
	out.append(hps_format(fTotal[2], num, sizeof(num)));
	out.append(" H/s\nHighest: ");
	out.append(hps_format(fHighestHps, num, sizeof(num)));
	out.append(" H/s\n");
	return out;
}

char* time_format(char* buf, size_t len, std::chrono::system_clock::time_point time)
{
	time_t ctime = std::chrono::system_clock::to_time_t(time);
	tm stime;

	/*
	 * Oh for god's sake... this feels like we are back to the 90's...
	 * and don't get me started on lack strcpy_s because NIH - use non-standard strlcpy...
	 * And of course C++ implements unsafe version because... reasons
	 */
	localtime_r(&ctime, &stime);
	strftime(buf, len, "%F %T", &stime);
	return buf;
}

std::string executor::result_report()
{
	std::string out;
	char num[128];
	char date[32];

	out.reserve(1024);

	size_t iGoodRes = vMineResults[0].count, iTotalRes = iGoodRes;
	size_t ln = vMineResults.size();

	for(size_t i=1; i < ln; i++)
		iTotalRes += vMineResults[i].count;

	out.append("RESULT REPORT\n");
	if(iTotalRes == 0)
	{
		out.append("You haven't found any results yet.\n");
		return "";
	}

	double dConnSec;
	{
		using namespace std::chrono;
		dConnSec = (double)duration_cast<seconds>(system_clock::now() - tPoolConnTime).count();
	}

	snprintf(num, sizeof(num), " (%.1f %%)\n", 100.0 * iGoodRes / iTotalRes);

	out.append("Difficulty       : ").append(std::to_string(iPoolDiff)).append(1, '\n');
	out.append("Good results     : ").append(std::to_string(iGoodRes)).append(" / ").
		append(std::to_string(iTotalRes)).append(num);

	if(iPoolCallTimes.size() != 0)
	{
		// Here we use iPoolCallTimes since it also gets reset when we disconnect
		snprintf(num, sizeof(num), "%.1f sec\n", dConnSec / iPoolCallTimes.size());
		out.append("Avg result time  : ").append(num);
	}
	out.append("Pool-side hashes : ").append(std::to_string(iPoolHashes)).append(2, '\n');
	out.append("Top 10 best results found:\n");

	for(size_t i=0; i < 10; i += 2)
	{
		snprintf(num, sizeof(num), "| %2llu | %16llu | %2llu | %16llu |\n",
			int_port(i), int_port(iTopDiff[i]), int_port(i+1), int_port(iTopDiff[i+1]));
		out.append(num);
	}

	out.append("\nError details:\n");
	if(ln > 1)
	{
		out.append("| Count | Error text                       | Last seen           |\n");
		for(size_t i=1; i < ln; i++)
		{
			snprintf(num, sizeof(num), "| %5llu | %-32.32s | %s |\n", int_port(vMineResults[i].count),
				vMineResults[i].msg.c_str(), time_format(date, sizeof(date), vMineResults[i].time));
			out.append(num);
		}
	}
	else
		out.append("Yay! No errors.\n");
	return "";
}

std::string executor::connection_report()
{
	std::string out;
	char num[128];
	char date[32];

	out.reserve(512);

	jpsock* pool = pool_ptr.get();

	out.append("CONNECTION REPORT\n");

	out.append("Pool address    : ").append(
			pool != nullptr ? system_constants::get_pool_pool_address() : "<not connected>"
	).append(1, '\n');

	if(pool != nullptr && pool->is_running() && pool->is_logged_in())
		out.append("Connected since : ").append(time_format(date, sizeof(date), tPoolConnTime)).append(1, '\n');
	else
		out.append("Connected since : <not connected>\n");

	size_t n_calls = iPoolCallTimes.size();
	if (n_calls > 1) {
		//Not-really-but-good-enough median
		std::nth_element(iPoolCallTimes.begin(), iPoolCallTimes.begin() + n_calls/2, iPoolCallTimes.end());
		out.append("Pool ping time  : ").append(std::to_string(iPoolCallTimes[n_calls/2])).append(" ms\n");
	}
	else
		out.append("Pool ping time  : (n/a)\n");

	out.append("\nNetwork error log:\n");
	size_t ln = vSocketLog.size();
	if(ln > 0)
	{
		out.append("| Date                | Error text                                             |\n");
		for(size_t i=0; i < ln; i++)
		{
			snprintf(num, sizeof(num), "| %s | %-54.54s |\n",
				time_format(date, sizeof(date), vSocketLog[i].time), vSocketLog[i].msg.c_str());
			out.append(num);
		}
	}
	else
		out.append("Yay! No errors.\n");
	return out;
}

void executor::print_report()
{
	std::cout << hashrate_report() << std::endl;
	std::cout << result_report() << std::endl;
	std::cout << connection_report() << std::endl;
}
