//
// Created by Snow Flake on 12/28/17.
//

#ifndef MONERO_CPU_MINER_GROESTL_HASH_H
#define MONERO_CPU_MINER_GROESTL_HASH_H

#include <cstring>
#include <stdint.h>

void do_groestl_hash(const uint8_t* input, std::size_t len, uint8_t* output);

#endif //MONERO_CPU_MINER_GROESTL_HASH_H
