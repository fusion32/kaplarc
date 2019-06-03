#ifndef CRYPTO_RSA_H_
#define CRYPTO_RSA_H_
#include "../def.h"
#include <gmp.h>

struct rsa_ctx{
	mpz_t p, q, n, e;	// key vars
	mpz_t dp, dq, qi;	// decoding vars
	mpz_t x0, x1, x2, x3;	// aux vars
	size_t encoding_limit;
};

void rsa_init(struct rsa_ctx *r);
void rsa_init_clone(struct rsa_ctx *r, struct rsa_ctx *from);
void rsa_cleanup(struct rsa_ctx *r);
bool rsa_setkey(struct rsa_ctx *r, const char *p, const char *q, const char *e);
void rsa_encode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *plen);
void rsa_decode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *plen);

#endif //CRYPTO_RSA_H_
