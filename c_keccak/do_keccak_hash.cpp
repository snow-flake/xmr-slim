//
// Created by Snow Flake on 12/28/17.
//

#include "do_keccak_hash.hpp"
#include "c_keccak.hpp"

// compute a keccak hash (md) of given byte length from "in"
int do_keccak(const uint8_t *in, int inlen, uint8_t *md, int mdlen) {
	return c_keccak::keccak(in, inlen, md, mdlen);
}

// update the state
void do_keccakf(uint64_t st[25], int norounds) {
	c_keccak::keccakf(st, norounds);
}
