//
// Created by Snow Flake on 12/28/17.
//

#ifndef C_KECCAK_DO_KECCAK_H
#define C_KECCAK_DO_KECCAK_H

#include <stdint.h>

// compute a keccak hash (md) of given byte length from "in"
int do_keccak(const uint8_t *in, int inlen, uint8_t *md, int mdlen);

// update the state
void do_keccakf(uint64_t st[25], int norounds);

#endif //C_KECCAK_DO_KECCAK_H
