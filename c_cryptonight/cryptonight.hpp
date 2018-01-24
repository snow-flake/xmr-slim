#ifndef __CRYPTONIGHT_H_INCLUDED
#define __CRYPTONIGHT_H_INCLUDED

#include <stddef.h>
#include <inttypes.h>

// define xmr settings
#define MONERO_MEMORY 2097152llu
#define MONERO_MASK 0x1FFFF0
#define MONERO_ITER 0x80000

typedef struct {
	uint8_t hash_state[224]; // Need only 200, explicit align
	uint8_t* long_state;
	uint8_t ctx_info[24]; //Use some of the extra memory for flags
} cryptonight_ctx;

typedef struct {
	const char* warning;
} alloc_msg;

cryptonight_ctx* cryptonight_alloc_ctx(size_t use_fast_mem, size_t use_mlock, alloc_msg* msg);

void cryptonight_free_ctx(cryptonight_ctx* ctx);

#endif
