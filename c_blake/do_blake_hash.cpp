//
// Created by Snow Flake on 12/28/17.
//

#include "c_blake256.h"
#include "do_blake_hash.hpp"

void do_blake_hash(const uint8_t* input, std::size_t len, uint8_t* output) {
	c_blake::blake256_hash(output, input, len);
}
