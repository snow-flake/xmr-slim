#ifndef C_BLAKE_DO_HASH_H
#define C_BLAKE_DO_HASH_H

#include <cstring>
#include <stdint.h>

void do_blake_hash(const uint8_t* input, std::size_t len, uint8_t* output);

#endif //C_BLAKE_DO_HASH_H