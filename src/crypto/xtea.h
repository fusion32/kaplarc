#ifndef CRYPTO_XTEA_H_
#define CRYPTO_XTEA_H_

#include "../def.h"
void xtea_encode(uint32 *k, uint8 *data, size_t len);
void xtea_decode(uint32 *k, uint8 *data, size_t len);

#endif //CRYPTO_XTEA_H_
