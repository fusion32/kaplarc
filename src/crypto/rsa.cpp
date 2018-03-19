#include "rsa.h"
#include <gmp.h>

struct rsa_ctx{
	mpz_t p, q, n, e;
	mpz_t dp, dq, qi;
	size_t limit;
};

//#define mpz_sizeinbytes(x) (mpz_size((x)) * sizeof(mp_limb_t))
#define mpz_sizeinbytes(x) ((mpz_sizeinbase((x), 2) + 7) >> 3)

struct rsa_ctx *rsa_create(void){
	struct rsa_ctx *r = (struct rsa_ctx*)malloc(sizeof(struct rsa_ctx));
	mpz_inits(r->p, r->q, r->n, r->e,
		r->dp, r->dq, r->qi, NULL);
	r->limit = 0;
	return r;
}

void rsa_destroy(struct rsa_ctx *r){
	mpz_clears(r->p, r->q, r->n, r->e,
		r->dp, r->dq, r->qi, NULL);
	free(r);
}

bool rsa_setkey(struct rsa_ctx *r, const char *p, const char *q, const char *e){
	bool ret = false;
	mpz_t aux, lambda, p_1, q_1;

	// set e, p, q, n
	mpz_set_str(r->p, p, 10);
	mpz_set_str(r->q, q, 10);
	mpz_set_str(r->e, e, 10);
	mpz_mul(r->n, r->p, r->q);

	// init local variables
	mpz_inits(aux, lambda, p_1, q_1, NULL);

	// calculate lambda(Carmichael's totient)
	mpz_sub_ui(p_1, r->p, 1);
	mpz_sub_ui(q_1, r->q, 1);
	mpz_lcm(lambda, p_1, q_1);

	// the coeficient `e` must be smaller than lambda
	if(mpz_cmp(lambda, r->e) <= 0)
		goto cleanup;

	// lambda and `e` must be coprimes
	mpz_gcd(aux, lambda, r->e);
	if(mpz_cmp_ui(aux, 1) != 0)
		goto cleanup;

	// calculate `d`
	mpz_invert(aux, r->e, lambda);

	// this next step is an optimization based on the
	// Chinese remainder theorem used for decoding
	mpz_mod(r->dp, aux, p_1);
	mpz_mod(r->dq, aux, q_1);
	mpz_invert(r->qi, r->q, r->p);

	// the maximum message length that can be encoded with this key
	r->limit = mpz_sizeinbytes(r->n) - 1;

	// return true
	ret = true;

cleanup:
	// clear local variables
	mpz_clears(aux, lambda, p_1, q_1, NULL);
	return ret;
}

bool rsa_can_encode(struct rsa_ctx *r, size_t len){
	return r->limit >= len;
}

size_t rsa_encoding_limit(struct rsa_ctx *r){
	return r->limit;
}

void rsa_encode(struct rsa_ctx *r, char *msg, size_t len, size_t *plen){
	mpz_t m, c;
	mpz_inits(m, c, NULL);
	mpz_import(m, len, 1, 1, 0, 0, msg);
	mpz_powm(c, m, r->e, r->n);
	mpz_export(msg, plen, 1, 1, 0, 0, c);
	mpz_clears(m, c, NULL);
}

void rsa_decode(struct rsa_ctx *r, char *msg, size_t len, size_t *plen){
	mpz_t c, m1, m2, h;
	mpz_inits(c, m1, m2, h, NULL);
	mpz_import(c, len, 1, 1, 0, 0, msg);
	mpz_powm(m1, c, r->dp, r->p);
	mpz_powm(m2, c, r->dq, r->q);
	mpz_sub(h, m1, m2);
	if(mpz_cmp(m1, m2) < 0) // m1 < m2
		mpz_add(h, h, r->p);
	mpz_mul(h, h, r->qi);
	mpz_mod(h, h, r->p);
	mpz_addmul(m2, h, r->q);
	mpz_export(msg, plen, 1, 1, 0, 0, m2);
	mpz_clears(c, m1, m2, h, NULL);
}
