#ifndef CRYPTO_BLOWFISH_H_
#define CRYPTO_BLOWFISH_H_
#include "../def.h"
struct blowfish_ctx{
	uint32 P[16 + 2];
	uint32 S[4 * 256];
};

void blowfish_init(struct blowfish_ctx *b);
void blowfish_setkey(struct blowfish_ctx *b,
			uint8 *key, uint32 len);
void blowfish_expandkey(struct blowfish_ctx *b,
			uint8 *key, uint32 len);
void blowfish_expandkey1(struct blowfish_ctx *b,
			uint8 *key, uint32 keylen,
			uint8 *data, uint32 datalen);
void blowfish_ecb_encode(struct blowfish_ctx *b, uint8 *data, uint32 len);
void blowfish_ecb_decode(struct blowfish_ctx *b, uint8 *data, uint32 len);
void blowfish_cbc_encode(struct blowfish_ctx *b, uint8 *iv, uint8 *data, uint32 len);
void blowfish_cbc_decode(struct blowfish_ctx *b, uint8 *iv, uint8 *data, uint32 len);

#endif //CRYPTO_BLOWFISH_H_
