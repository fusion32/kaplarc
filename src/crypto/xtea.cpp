#include "../endian.h"
#include "xtea.h"

#define ROUND_TO_8(x) (((x) + 7) & ~7)
#define GET_U32(x, d, i)					\
	(x) = ((d)[(i)+0] << 24)				\
		| ((d)[(i)+1] << 16)				\
		| ((d)[(i)+2] << 8)				\
		| ((d)[(i)+3])
#define PUT_U32(x, d, i)					\
	do{							\
		(d)[(i)+0] = ((x) >> 24) & 0xFF;		\
		(d)[(i)+1] = ((x) >> 16) & 0xFF;		\
		(d)[(i)+2] = ((x) >> 8) & 0xFF;			\
		(d)[(i)+3] = (x) & 0xFF;			\
	}while(0)

void xtea_encode(uint32 *k, uint8 *data, uint32 len){
	uint32 v0, v1, delta, sum, i;
	len = ROUND_TO_8(len);
	while(len > 0){
		GET_U32(v0, data, 0);
		GET_U32(v1, data, 4);
		delta = 0x9E3779B9UL; sum = 0UL;
		for(i = 0; i < 32; ++i){
			v0 += ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
			sum += delta;
			v1 += ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		PUT_U32(v0, data, 0);
		PUT_U32(v1, data, 4);
		len -= 8; data += 8;
	}
}

void xtea_decode(uint32 *k, uint8 *data, uint32 len){
	uint32 v0, v1, delta, sum, i;
	len = ROUND_TO_8(len);
	while(len > 0){
		GET_U32(v0, data, 0);
		GET_U32(v1, data, 4);
		delta = 0x9E3779B9UL; sum = 0xC6EF3720UL;
		for(i = 0; i < 32; ++i){
			v1 -= ((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum -= delta;
			v0 -= ((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]);
		}
		PUT_U32(v0, data, 0);
		PUT_U32(v1, data, 4);
		len -= 8; data += 8;
	}
}
