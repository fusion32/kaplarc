#ifndef CRYPTO_BLOWFISH_H_
#define CRYPTO_BLOWFISH_H_
#include "../def.h"
struct blowfish_ctx{
	uint32 P[16 + 2];
	uint32 S[4 * 256];
};

void blowfish_setkey(struct blowfish_ctx *b, const char *key, uint32 len);
void blowfish_encode_be(struct blowfish_ctx *b, uint32 *msg, uint32 len);
void blowfish_encode_le(struct blowfish_ctx *b, uint32 *msg, uint32 len);
void blowfish_decode_be(struct blowfish_ctx *b, uint32 *msg, uint32 len);
void blowfish_decode_le(struct blowfish_ctx *b, uint32 *msg, uint32 len);

#endif //CRYPTO_BLOWFISH_H_
