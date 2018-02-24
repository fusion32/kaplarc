#include "../endian.h"
#include "xtea.h"

void xtea_encode_be(uint32 *k, uint32 *input, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_be(input[0]);
		v1 = u32_from_be(input[1]);
		delta = 0x9E3779B9; sum = 0;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		input[0] = u32_to_be(v0);
		input[1] = u32_to_be(v1);
		len -= 2; input += 2;
	}
}

void xtea_encode_le(uint32 *k, uint32 *input, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_le(input[0]);
		v1 = u32_from_le(input[1]);
		delta = 0x9E3779B9; sum = 0;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		input[0] = u32_to_le(v0);
		input[1] = u32_to_le(v1);
		len -= 2; input += 2;
	}
}

void xtea_decode_be(uint32 *k, uint32 *input, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_be(input[0]);
		v1 = u32_from_be(input[1]);
		delta = 0x9E3779B9; sum = 0xC6EF3720;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		input[0] = u32_to_be(v0);
		input[1] = u32_to_be(v1);
		len -= 2; input += 2;
	}
}

void xtea_decode_le(uint32 *k, uint32 *input, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_le(input[0]);
		v1 = u32_from_le(input[1]);
		delta = 0x9E3779B9; sum = 0xC6EF3720;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		input[0] = u32_to_le(v0);
		input[1] = u32_to_le(v1);
		len -= 2; input += 2;
	}
}
