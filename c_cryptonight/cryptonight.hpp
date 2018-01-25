#ifndef __CRYPTONIGHT_H_INCLUDED
#define __CRYPTONIGHT_H_INCLUDED

#include <stddef.h>
#include <inttypes.h>
#include <iostream>

// define xmr settings
#define MONERO_MEMORY 2097152llu
#define MONERO_MASK 0x1FFFF0
#define MONERO_ITER 0x80000

struct cryptonight_ctx{
	uint8_t hash_state[224]; // Need only 200, explicit align
	uint8_t* long_state;
	uint8_t ctx_info[24]; //Use some of the extra memory for flags
};

typedef struct {
	const char* warning;
} alloc_msg;

cryptonight_ctx* cryptonight_alloc_ctx(size_t use_fast_mem, size_t use_mlock, alloc_msg* msg);

void cryptonight_free_ctx(cryptonight_ctx* ctx);

class cryptonight_ctx_cls {
public:
	cryptonight_ctx *ctx;

	cryptonight_ctx_cls(size_t use_fast_mem = 1, size_t use_mlock = 1): ctx(nullptr) {
		alloc_msg msg;
		ctx = cryptonight_alloc_ctx(use_fast_mem, use_mlock, &msg);
		if (msg.warning != NULL) {
			std::cout << __FILE__ << ":" << __LINE__ << ": Memory alloc failed: " << msg.warning << std::endl;
		}
	}

	~cryptonight_ctx_cls() {
		if (ctx != nullptr) {
			cryptonight_free_ctx(ctx);
		}
		ctx = nullptr;
	}


};

#endif
