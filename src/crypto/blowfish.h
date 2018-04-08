#ifndef CRYPTO_BLOWFISH_H_
#define CRYPTO_BLOWFISH_H_
#include "../def.h"
struct blowfish_ctx{
	uint32 P[16 + 2];
	uint32 S[4 * 256];
};

void blowfish_init(struct blowfish_ctx *b);
void blowfish_setkey(struct blowfish_ctx *b,
			uint8 *key, size_t len);
void blowfish_expandkey(struct blowfish_ctx *b,
			uint8 *key, size_t len);
void blowfish_expandkey1(struct blowfish_ctx *b,
			uint8 *key, size_t keylen,
			uint8 *data, size_t datalen);
void blowfish_ecb_encode(struct blowfish_ctx *b, uint8 *data, size_t len);
void blowfish_ecb_decode(struct blowfish_ctx *b, uint8 *data, size_t len);
void blowfish_cbc_encode(struct blowfish_ctx *b, uint8 *iv, uint8 *data, size_t len);
void blowfish_cbc_decode(struct blowfish_ctx *b, uint8 *iv, uint8 *data, size_t len);

// this helper function is useful for implementing bcrypt
void blowfish_ecb_encode_n(struct blowfish_ctx *b, int n, uint8 *data, size_t len);

#endif //CRYPTO_BLOWFISH_H_
