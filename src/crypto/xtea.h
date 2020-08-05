#ifndef KAPLAR_CRYPTO_XTEA_H_
#define KAPLAR_CRYPTO_XTEA_H_ 1

#include "../common.h"
void xtea_encode(uint32 *k, uint8 *data, size_t len);
void xtea_decode(uint32 *k, uint8 *data, size_t len);

#endif //KAPLAR_CRYPTO_XTEA_H_
