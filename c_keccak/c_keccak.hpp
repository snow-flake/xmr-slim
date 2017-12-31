// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

#ifndef MONERO_CPU_MINER_C_KECCAK_H
#define MONERO_CPU_MINER_C_KECCAK_H

#include <stdint.h>
#include <string.h>

namespace c_keccak {

// compute a keccak hash (md) of given byte length from "in"
	int keccak(const uint8_t *in, int inlen, uint8_t *md, int mdlen);

// update the state
	void keccakf(uint64_t st[25], int norounds);

	void keccak1600(const uint8_t *in, int inlen, uint8_t *md);

}

#endif //MONERO_CPU_MINER_C_KECCAK_H
