//
// MurmurHash2, by Austin Appleby (aappleby (AT) gmail)
// https://sites.google.com/site/murmurhash/
//

#include "hash.h"
#include "buffer_util.h"

static uint32 __decode_u32(const void *data){
#if ARCH_BIG_ENDIAN
	return decode_u32_be((void*)data);
#else
	return decode_u32_le((void*)data);
#endif
}

// murmur2_32 hash
#define MURMUR2_M 0x5bd1e995
uint32 murmur2_32(const uint8 *data, size_t len, uint32 seed){
	uint32 h = (uint32)len ^ seed;
	uint32 k;

	// mix 4 bytes at a time
	while(len >= 4){
		k = __decode_u32(data);
		k *= MURMUR2_M;
		k ^= k >> 24;
		k *= MURMUR2_M;
		h *= MURMUR2_M;
		h ^= k;
		data += 4;
		len -= 4;
	}
	// mix last few bytes
	switch(len){
	case 3:	h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= MURMUR2_M;
	}
	// last hash mix
	h ^= h >> 13;
	h *= MURMUR2_M;
	h ^= h >> 15;
	return h;
}
