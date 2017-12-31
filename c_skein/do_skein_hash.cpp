//
// Created by Snow Flake on 12/28/17.
//

#include "do_skein_hash.hpp"
#include "c_skein.hpp"

void do_skein_hash(const uint8_t* input, std::size_t len, uint8_t* output) {
	c_skein::skein_hash(8 * 32, input, 8 * len, output);
}
