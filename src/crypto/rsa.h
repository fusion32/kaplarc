#ifndef CRYPTO_RSA_H_
#define CRYPTO_RSA_H_
#include <gmp.h>
struct rsa_ctx{
	mpz_t p, q, n, e;
	mpz_t dp, dq, qi;
	size_t limit;
};

void rsa_init(struct rsa_ctx *r);
void rsa_clear(struct rsa_ctx *r);
bool rsa_setkey(struct rsa_ctx *r, const char *p, const char *q, const char *e);
bool rsa_can_encode(struct rsa_ctx *r, size_t len);
size_t rsa_encoding_limit(struct rsa_ctx *r);
void rsa_encode(struct rsa_ctx *r, char *msg, size_t len, size_t *plen);
void rsa_decode(struct rsa_ctx *r, char *msg, size_t len, size_t *plen);

#endif //CRYPTO_RSA_H_
