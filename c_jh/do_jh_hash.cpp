//
// Created by Snow Flake on 12/28/17.
//

#include "do_jh_hash.hpp"
#include "c_jh.hpp"

void do_jh_hash(const uint8_t* input, std::size_t len, uint8_t* output) {
	c_jh::jh_hash(32 * 8, input, 8 * len, output);
}
