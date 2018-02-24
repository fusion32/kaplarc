#ifndef ENDIAN_H_
#define ENDIAN_H_

#include "def.h"

static inline uint16 swap_u16(uint16 x){
	return (x & 0xFF00) >> 8
		| (x & 0x00FF) << 8;
}
static inline uint32 swap_u32(uint32 x){
	return (x & 0xFF000000) >> 24
		| (x & 0x00FF0000) >> 8
		| (x & 0x0000FF00) << 8
		| (x & 0x000000FF) << 24;
}
static inline uint64 swap_u64(uint64 x){
	return (x & 0xFF00000000000000) >> 56
		| (x & 0x00FF000000000000) >> 40
		| (x & 0x0000FF0000000000) >> 24
		| (x & 0x000000FF00000000) >> 8
		| (x & 0x00000000FF000000) << 8
		| (x & 0x0000000000FF0000) << 24
		| (x & 0x000000000000FF00) << 40
		| (x & 0x00000000000000FF) << 56;
}

#ifdef __BIG_ENDIAN__
#define u16_from_be(x)	(x)
#define u16_from_le(x)	swap_u16(x)
#define u32_from_be(x)	(x)
#define u32_from_le(x)	swap_u32(x)
#define u64_from_be(x)	(x)
#define u64_from_le(x)	swap_u64(x)
#define u16_to_be(x)	(x)
#define u16_to_le(x)	swap_u16(x)
#define u32_to_be(x)	(x)
#define u32_to_le(x)	swap_u32(x)
#define u64_to_be(x)	(x)
#define u64_to_le(x)	swap_u64(x)
#else //__BIG_ENDIAN__
#define u16_from_be(x)	swap_u16(x)
#define u16_from_le(x)	(x)
#define u32_from_be(x)	swap_u32(x)
#define u32_from_le(x)	(x)
#define u64_from_be(x)	swap_u64(x)
#define u64_from_le(x)	(x)
#define u16_to_be(x)	swap_u16(x)
#define u16_to_le(x)	(x)
#define u32_to_be(x)	swap_u32(x)
#define u32_to_le(x)	(x)
#define u64_to_be(x)	swap_u64(x)
#define u64_to_le(x)	(x)
#endif //__BIG_ENDIAN__

#endif //ENDIAN_H_
