#include "rsa.h"
#include <gmp.h>
#include <stdlib.h>

//#define mpz_sizeinbytes(x) (mpz_size((x)) * sizeof(mp_limb_t))
#define mpz_sizeinbytes(x) ((mpz_sizeinbase((x), 2) + 7) >> 3)

struct RSA{
	mpz_t p, q, n, e;
	mpz_t dp, dq, qi;
	size_t limit;
};

void rsa_init(struct RSA **rsa){
	struct RSA *r = *rsa = (struct RSA*)malloc(sizeof(struct RSA));
	mpz_inits(r->p, r->q, r->n, r->e,
		r->dp, r->dq, r->qi, NULL);
	r->limit = 0;
}

void rsa_clear(struct RSA *rsa){
	mpz_clears(rsa->p, rsa->q, rsa->n, rsa->e,
		rsa->dp, rsa->dq, rsa->qi, NULL);
	free(rsa);
}

bool rsa_setkey(struct RSA *rsa, const char *p, const char *q, const char *e){
	bool ret = false;
	mpz_t aux, lambda, p_1, q_1;

	// set e, p, q, n
	mpz_set_str(rsa->p, p, 10);
	mpz_set_str(rsa->q, q, 10);
	mpz_set_str(rsa->e, e, 10);
	mpz_mul(rsa->n, rsa->p, rsa->q);

	// init local variables
	mpz_inits(aux, lambda, p_1, q_1, NULL);

	// calculate lambda(Carmichael's totient)
	mpz_sub_ui(p_1, rsa->p, 1);
	mpz_sub_ui(q_1, rsa->q, 1);
	mpz_lcm(lambda, p_1, q_1);

	// the coeficient `e` must be smaller than lambda
	if(mpz_cmp(lambda, rsa->e) <= 0)
		goto cleanup;

	// lambda and `e` must be coprimes
	mpz_gcd(aux, lambda, rsa->e);
	if(mpz_cmp_ui(aux, 1) != 0)
		goto cleanup;

	// calculate `d`
	mpz_invert(aux, rsa->e, lambda);

	// this next step is an optimization based on the
	// Chinese remainder theorem used for decoding
	mpz_mod(rsa->dp, aux, p_1);
	mpz_mod(rsa->dq, aux, q_1);
	mpz_invert(rsa->qi, rsa->q, rsa->p);

	// the maximum message length that can be encoded with this key
	rsa->limit = mpz_sizeinbytes(rsa->n) - 1;

	// return true
	ret = true;

cleanup:
	// clear local variables
	mpz_clears(aux, lambda, p_1, q_1, NULL);
	return ret;
}

bool rsa_can_encode(struct RSA *rsa, size_t len){
	return rsa->limit >= len;
}

size_t rsa_encoding_limit(struct RSA *rsa){
	return rsa->limit;
}

void rsa_encode(struct RSA *rsa, char *msg, size_t len, size_t *plen){
	mpz_t m, c;
	mpz_inits(m, c, NULL);
	mpz_import(m, len, 1, 1, 0, 0, msg);
	mpz_powm(c, m, rsa->e, rsa->n);
	mpz_export(msg, plen, 1, 1, 0, 0, c);
	mpz_clears(m, c, NULL);
}

void rsa_decode(struct RSA *rsa, char *msg, size_t len, size_t *plen){
	mpz_t c, m1, m2, h;
	mpz_inits(c, m1, m2, h, NULL);
	mpz_import(c, len, 1, 1, 0, 0, msg);
	mpz_powm(m1, c, rsa->dp, rsa->p);
	mpz_powm(m2, c, rsa->dq, rsa->q);
	mpz_sub(h, m1, m2);
	if(mpz_cmp(m1, m2) < 0) // m1 < m2
		mpz_add(h, h, rsa->p);
	mpz_mul(h, h, rsa->qi);
	mpz_mod(h, h, rsa->p);
	mpz_addmul(m2, h, rsa->q);
	mpz_export(msg, plen, 1, 1, 0, 0, m2);
	mpz_clears(c, m1, m2, h, NULL);
}
