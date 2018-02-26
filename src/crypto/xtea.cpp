#include "../endian.h"
#include "xtea.h"

void xtea_encode_be(uint32 *k, uint32 *msg, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_be(msg[0]);
		v1 = u32_from_be(msg[1]);
		delta = 0x9E3779B9UL; sum = 0UL;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		msg[0] = u32_to_be(v0);
		msg[1] = u32_to_be(v1);
		len -= 2; msg += 2;
	}
}

void xtea_encode_le(uint32 *k, uint32 *msg, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_le(msg[0]);
		v1 = u32_from_le(msg[1]);
		delta = 0x9E3779B9UL; sum = 0UL;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		msg[0] = u32_to_le(v0);
		msg[1] = u32_to_le(v1);
		len -= 2; msg += 2;
	}
}

void xtea_decode_be(uint32 *k, uint32 *msg, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_be(msg[0]);
		v1 = u32_from_be(msg[1]);
		delta = 0x9E3779B9UL; sum = 0xC6EF3720UL;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		msg[0] = u32_to_be(v0);
		msg[1] = u32_to_be(v1);
		len -= 2; msg += 2;
	}
}

void xtea_decode_le(uint32 *k, uint32 *msg, uint32 len){
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = u32_from_le(msg[0]);
		v1 = u32_from_le(msg[1]);
		delta = 0x9E3779B9UL; sum = 0xC6EF3720UL;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		msg[0] = u32_to_le(v0);
		msg[1] = u32_to_le(v1);
		len -= 2; msg += 2;
	}
}
