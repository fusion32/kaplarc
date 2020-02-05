#ifndef KAPLAR_HASH_H_
#define KAPLAR_HASH_H_

#include "def.h"

// adler32.c
uint32 adler32(const uint8 *buf, size_t len);
// murmur2.c
uint32 murmur2_32(const uint8 *data, size_t len, uint32 seed);

#endif //KAPLAR_HASH_H_
