#ifndef CRYPTO_XTEA_H_
#define CRYPTO_XTEA_H_

#include "../def.h"
void xtea_encode_be(uint32 *k, uint32 *msg, uint32 len);
void xtea_encode_le(uint32 *k, uint32 *msg, uint32 len);
void xtea_decode_be(uint32 *k, uint32 *msg, uint32 len);
void xtea_decode_le(uint32 *k, uint32 *msg, uint32 len);

#endif //CRYPTO_XTEA_H_
