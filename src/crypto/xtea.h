#ifndef CRYPTO_XTEA_H_
#define CRYPTO_XTEA_H_

#include "../def.h"
void xtea_encode(uint32 *k, uint8 *data, uint32 len);
void xtea_decode(uint32 *k, uint8 *data, uint32 len);

#endif //CRYPTO_XTEA_H_
