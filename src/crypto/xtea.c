#include "xtea.h"
#include "../buffer_util.h"

#define IS_MULT_OF_8(x) (((x) & 7) == 0)

void xtea_encode(uint32 *k, uint8 *data, size_t len){
	DEBUG_ASSERT(IS_MULT_OF_8(len));
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = decode_u32_le(data);
		v1 = decode_u32_le(data + 4);
		delta = 0x9E3779B9UL; sum = 0UL;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		encode_u32_le(data, v0);
		encode_u32_le(data + 4, v1);
		len -= 8; data += 8;
	}
}

void xtea_decode(uint32 *k, uint8 *data, size_t len){
	DEBUG_ASSERT(IS_MULT_OF_8(len));
	uint32 v0, v1, delta, sum, i;
	while(len > 0){
		v0 = decode_u32_le(data);
		v1 = decode_u32_le(data + 4);
		delta = 0x9E3779B9UL; sum = 0xC6EF3720UL;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		encode_u32_le(data, v0);
		encode_u32_le(data + 4, v1);
		len -= 8; data += 8;
	}
}
