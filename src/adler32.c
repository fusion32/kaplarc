//
// Adler32, by Mark Adler
//

#include "hash.h"

#define BASE 65521U
#define NMAX 5552

#define DO1(buf, i)	{a += buf[i]; b += a;}
#define DO2(buf, i)	DO1(buf,i); DO1(buf,i+1);
#define DO4(buf, i)	DO2(buf,i); DO2(buf,i+2);
#define DO8(buf, i)	DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)	DO8(buf,0); DO8(buf,8);

uint32 adler32(const uint8 *buf, size_t len){
	uint32 a = 1;
	uint32 b = 0;
	int k;
	while(len > 0){
		k = (int)(len > NMAX ? NMAX : len);
		len -= k;
		while(k >= 16){
			DO16(buf);
			buf += 16;
			k -= 16;
		}
		while(k-- != 0){
			a += *buf++;
			b += a;
		}
		a %= BASE;
		b %= BASE;
	}
	return a | (b << 16);
}
