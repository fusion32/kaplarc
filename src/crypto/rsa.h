#ifndef CRYPTO_RSA_H_
#define CRYPTO_RSA_H_
#include "../def.h"
struct rsa_ctx;
struct rsa_ctx *rsa_create(void);
void rsa_destroy(struct rsa_ctx *r);
bool rsa_setkey(struct rsa_ctx *r, const char *p, const char *q, const char *e);
bool rsa_can_encode(struct rsa_ctx *r, size_t len);
size_t rsa_encoding_limit(struct rsa_ctx *r);
void rsa_encode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *plen);
void rsa_decode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *plen);

#endif //CRYPTO_RSA_H_
