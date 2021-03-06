#pragma once

#include "c_cryptonight/cryptonight.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/net/msgstruct.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

namespace xmrstak
{
namespace cpu
{

class minethd : public iBackend
{
public:
	static std::vector<iBackend*> thread_starter(uint32_t threadOffset, msgstruct::miner_work& pWork);
	static bool self_test();
	static bool thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id);
	static cryptonight_ctx* minethd_alloc_ctx();

private:
	typedef void (*cn_hash_fun_multi)(const void*, size_t, void*, cryptonight_ctx**);

	minethd(msgstruct::miner_work& pWork, size_t iNo, int iMultiway, int64_t affinity);

	template<size_t N>
	void multiway_work_main(cn_hash_fun_multi hash_fun_multi);

	template<size_t N>
	void prep_multiway_work(uint8_t *bWorkBlob, uint32_t **piNonce);

	void single_work_main();
	void double_work_main();
	void triple_work_main();
	void quad_work_main();
	void penta_work_main();

	void consume_work();

	uint64_t iJobNo;

	static msgstruct::miner_work oGlobalWork;
	msgstruct::miner_work oWork;

	std::promise<void> order_fix;
	std::mutex thd_aff_set;

	std::thread oWorkThd;
	int64_t affinity;

	bool bQuit;
};

} // namespace cpu
} // namepsace xmrstak
