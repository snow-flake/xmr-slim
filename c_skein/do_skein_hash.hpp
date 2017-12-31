//
// Created by Snow Flake on 12/28/17.
//

#ifndef C_SKEIN_DO_SKEIN_H
#define C_SKEIN_DO_SKEIN_H

#include <cstring>
#include <stdint.h>

void do_skein_hash(const uint8_t* input, std::size_t len, uint8_t* output);

#endif //MONERO_CPU_MINER_DO_SKEIN_H
