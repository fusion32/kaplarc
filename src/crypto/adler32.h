#ifndef CRYPTO_ADLER32_H_
#define CRYPTO_ADLER32_H_

#include "../def.h"
uint32 adler32(const uint8 *buf, size_t len);
unsigned long adler32(const unsigned char *buf, unsigned long len);

#endif //CRYPTO_ADLER32_H_
