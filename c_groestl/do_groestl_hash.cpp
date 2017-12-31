//
// Created by Snow Flake on 12/28/17.
//

#include "do_groestl_hash.hpp"
#include "c_groestl.hpp"

void do_groestl_hash(const uint8_t* input, std::size_t len, uint8_t* output) {
	c_groestl::groestl(input, len * 8, output);
}
