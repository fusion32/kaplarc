#ifndef BUFFER_UTIL_H_
#define BUFFER_UTIL_H_

#include "def.h"
#include "endian.h"

static INLINE void encode_u8(uint8 *data, uint8 val){
	data[0] = val;
}

static INLINE uint8 decode_u8(uint8 *data){
	return data[0];
}

#ifdef ARCH_UNALIGNED_ACCESS
static INLINE void encode_u16_be(uint8 *data, uint16 val){
	*(uint16*)(data) = u16_cpu_to_be(val);
}

static INLINE uint16 decode_u16_be(uint8 *data){
	uint16 val = *(uint16*)(data);
	return u16_be_to_cpu(val);
}

static INLINE void encode_u16_le(uint8 *data, uint16 val){
	*(uint16*)(data) = u16_cpu_to_le(val);
}

static INLINE uint16 decode_u16_le(uint8 *data){
	uint16 val = *(uint16*)(data);
	return u16_le_to_cpu(val);
}

static INLINE void encode_u32_be(uint8 *data, uint32 val){
	*(uint32*)(data) = u32_cpu_to_be(val);
}

static INLINE uint32 decode_u32_be(uint8 *data){
	uint32 val = *(uint32*)(data);
	return u32_be_to_cpu(val);
}

static INLINE void encode_u32_le(uint8 *data, uint32 val){
	*(uint32*)(data) = u32_cpu_to_le(val);
}

static INLINE uint32 decode_u32_le(uint8 *data){
	uint32 val = *(uint32*)(data);
	return u32_le_to_cpu(val);
}

static INLINE void encode_u64_be(uint8 *data, uint64 val){
	*(uint64*)(data) = u64_cpu_to_be(val);
}

static INLINE uint64 decode_u64_be(uint8 *data){
	uint64 val = *(uint64*)(data);
	return u64_be_to_cpu(val);
}

static INLINE void encode_u64_le(uint8 *data, uint64 val){
	*(uint64*)(data) = u64_cpu_to_le(val);
}

static INLINE uint64 decode_u64_le(uint8 *data){
	uint64 val = *(uint64*)(data);
	return u64_le_to_cpu(val);
}

#else //ARCH_UNALIGNED_ACCESS

static INLINE void encode_u16_be(uint8 *data, uint16 val){
	data[0] = (uint8)(val >> 8);
	data[1] = (uint8)(val);
}

static INLINE uint16 decode_u16_be(uint8 *data){
	return ((uint16)(data[0]) << 8) |
		((uint16)(data[1]));
}

static INLINE void encode_u16_le(uint8 *data, uint16 val){
	data[0] = (uint8)(val);
	data[1] = (uint8)(val >> 8);
}

static INLINE uint16 decode_u16_le(uint8 *data){
	return ((uint16)(data[0])) |
		((uint16)(data[1]) << 8);
}

static INLINE void encode_u32_be(uint8 *data, uint32 val){
	data[0] = (uint8)(val >> 24);
	data[1] = (uint8)(val >> 16);
	data[2] = (uint8)(val >> 8);
	data[3] = (uint8)(val);
}

static INLINE uint32 decode_u32_be(uint8 *data){
	return ((uint32)(data[0]) << 24) |
		((uint32)(data[1]) << 16) |
		((uint32)(data[2]) << 8) |
		((uint32)(data[3]));
}

static INLINE void encode_u32_le(uint8 *data, uint32 val){
	data[0] = (uint8)(val);
	data[1] = (uint8)(val >> 8);
	data[2] = (uint8)(val >> 16);
	data[3] = (uint8)(val >> 24);
}

static INLINE uint32 decode_u32_le(uint8 *data){
	return ((uint32)(data[0])) |
		((uint32)(data[1]) << 8) |
		((uint32)(data[2]) << 16) |
		((uint32)(data[3]) << 24);
}

static INLINE void encode_u64_be(uint8 *data, uint64 val){
	data[0] = (uint8)(val >> 56);
	data[1] = (uint8)(val >> 48);
	data[2] = (uint8)(val >> 40);
	data[3] = (uint8)(val >> 32);
	data[4] = (uint8)(val >> 24);
	data[5] = (uint8)(val >> 16);
	data[6] = (uint8)(val >> 8);
	data[7] = (uint8)(val);
}

static INLINE uint64 decode_u64_be(uint8 *data){
	return ((uint64)(data[0]) << 56) |
		((uint64)(data[1]) << 48) |
		((uint64)(data[2]) << 40) |
		((uint64)(data[3]) << 32) |
		((uint64)(data[4]) << 24) |
		((uint64)(data[5]) << 16) |
		((uint64)(data[6]) << 8) |
		((uint64)(data[7]));
}

static INLINE void encode_u64_le(uint8 *data, uint64 val){
	data[0] = (uint8)(val);
	data[1] = (uint8)(val >> 8);
	data[2] = (uint8)(val >> 16);
	data[3] = (uint8)(val >> 24);
	data[4] = (uint8)(val >> 32);
	data[5] = (uint8)(val >> 40);
	data[6] = (uint8)(val >> 48);
	data[7] = (uint8)(val >> 56);
}

static INLINE uint64 decode_u64_le(uint8 *data){
	return ((uint64)(data[0])) |
		((uint64)(data[1]) << 8) |
		((uint64)(data[2]) << 16) |
		((uint64)(data[3]) << 24) |
		((uint64)(data[4]) << 32) |
		((uint64)(data[5]) << 40) |
		((uint64)(data[6]) << 48) |
		((uint64)(data[7]) << 56);
}

#endif //ARCH_UNALIGNED_ACCESS

static INLINE void encode_f32_be(uint8 *data, float val){
	uint32 u32_val = *(uint32*)(&val);
	encode_u32_be(data, u32_val);
}

static INLINE float decode_f32_be(uint8 *data){
	uint32 u32_val = decode_u32_be(data);
	float val = *(float*)(&u32_val);
	return val;
}

static INLINE void encode_f32_le(uint8 *data, float val){
	uint32 u32_val = *(uint32*)(&val);
	encode_u32_le(data, u32_val);
}

static INLINE float decode_f32_le(uint8 *data){
	uint32 u32_val = decode_u32_le(data);
	float val = *(float*)(&u32_val);
	return val;
}

static INLINE void encode_f64_be(uint8 *data, double val){
	uint64 u64_val = *(uint64*)(&val);
	encode_u64_be(data, u64_val);
}

static INLINE double decode_f64_be(uint8 *data){
	uint64 u64_val = decode_u64_be(data);
	double val = *(double*)(&u64_val);
	return val;
}

static INLINE void encode_f64_le(uint8 *data, double val){
	uint64 u64_val = *(uint64*)(&val);
	encode_u64_le(data, u64_val);
}

static INLINE double decode_f64_le(uint8 *data){
	uint64 u64_val = decode_u64_le(data);
	double val = *(double*)(&u64_val);
	return val;
}

#endif //BUFFER_UTIL_H_
