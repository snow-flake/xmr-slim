#ifndef MONERO_CPU_MINER_GROESTL_GROESTL_H
#define MONERO_CPU_MINER_GROESTL_GROESTL_H
#include <stdint.h>

/* some sizes (number of bytes) */
#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8
#define SIZE512 (ROWS*COLS512)

namespace c_groestl {
    typedef unsigned char BitSequence;
    typedef unsigned long long DataLength;
    enum HashReturn {
        SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2
    };

    struct groestlHashState {
        uint32_t chaining[SIZE512 / sizeof(uint32_t)];            /* actual state */
        uint32_t block_counter1, block_counter2;         /* message block counter(s) */
        BitSequence buffer[SIZE512];      /* data buffer */
        int buf_ptr;              /* data buffer pointer */
        int bits_in_last_byte;    /* no. of message bits in last byte of data buffer */
    };

    void groestl(const BitSequence *, DataLength, BitSequence *);
}

#endif //MONERO_CPU_MINER_GROESTL_GROESTL_H
