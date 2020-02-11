#include "rsa.h"

//#define mpz_sizeinbytes(x) (mpz_size((x)) * sizeof(mp_limb_t))
#define mpz_sizeinbytes(x) ((mpz_sizeinbase((x), 2) + 7) >> 3)

void rsa_init(struct rsa_ctx *r){
	mpz_inits(r->p, r->q, r->n, r->e,
		r->dp, r->dq, r->qi,
		r->x0, r->x1, r->x2, r->x3, NULL);
	r->encoding_limit = 0;
}

void rsa_init_clone(struct rsa_ctx *r, struct rsa_ctx *from){
	mpz_init_set(r->p, from->p);
	mpz_init_set(r->q, from->q);
	mpz_init_set(r->n, from->n);
	mpz_init_set(r->e, from->e);
	mpz_init_set(r->dp, from->dp);
	mpz_init_set(r->dq, from->dq);
	mpz_init_set(r->qi, from->qi);
	mpz_inits(r->x0, r->x1, r->x2, r->x3, NULL); // these don't need to be copied
	r->encoding_limit = from->encoding_limit;
}

void rsa_cleanup(struct rsa_ctx *r){
	mpz_clears(r->p, r->q, r->n, r->e,
		r->dp, r->dq, r->qi,
		r->x0, r->x1, r->x2, r->x3, NULL);
	r->encoding_limit = 0;
}

bool rsa_setkey(struct rsa_ctx *r, const char *p, const char *q, const char *e){
	// set p, q, n, e
	mpz_set_str(r->p, p, 10);
	mpz_set_str(r->q, q, 10);
	mpz_set_str(r->e, e, 10);
	mpz_mul(r->n, r->p, r->q);

	// calculate the Carmichael's totient (lambda)
	mpz_sub_ui(r->x0, r->p, 1);	// x0 = p - 1
	mpz_sub_ui(r->x1, r->q, 1);	// x1 = q - 1
	mpz_lcm(r->x2, r->x0, r->x1);	// x2 = lcm(x0, x1) <-- lambda

	// `e` must be smaller than lambda
	if(mpz_cmp(r->x2, r->e) <= 0)
		return false;

	// lambda and `e` must be coprimes
	mpz_gcd(r->x3, r->x2, r->e);	// x3 = gcd(x2, e)
	if(mpz_cmp_ui(r->x3, 1) != 0)
		return false;

	// calculate `d`
	mpz_invert(r->x3, r->e, r->x2);	// x3 = invert(e, x2)

	// this next step is an optimization based on the
	// Chinese remainder theorem used for decoding
	mpz_mod(r->dp, r->x3, r->x0);	// dp = x3 mod (p - 1)
	mpz_mod(r->dq, r->x3, r->x1);	// dq = x3 mod (q - 1)
	mpz_invert(r->qi, r->q, r->p);	// qi = invert(q, p)

	// roughly the maximum message length that can
	// be encoded with this key
	r->encoding_limit = mpz_sizeinbytes(r->n);

	return true;
}

void rsa_encode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *outlen){
	mpz_import(r->x0, len, 1, 1, 0, 0, data);	// x0 = import(data)
	mpz_powm(r->x1, r->x0, r->e, r->n);		// x1 = (x0 ^ e) mod n
	mpz_export(data, outlen, 1, 1, 0, 0, r->x1);	// data = export(x1);
}

void rsa_decode(struct rsa_ctx *r, uint8 *data, size_t len, size_t *outlen){
	mpz_import(r->x0, len, 1, 1, 0, 0, data);	// x0 = import(data)
	mpz_powm(r->x1, r->x0, r->dp, r->p);		// x1 = (x0 ^ dp) mod p
	mpz_powm(r->x2, r->x0, r->dq, r->q);		// x2 = (x0 ^ dq) mod q
	mpz_sub(r->x3, r->x1, r->x2);			// x3 = x1 - x2
	if(mpz_cmp(r->x1, r->x2) < 0)			// if x1 < x2
		mpz_add(r->x3, r->x3, r->p);		//	x3 = x3 + p
	mpz_mul(r->x3, r->x3, r->qi);			//
	mpz_mod(r->x3, r->x3, r->p);			// x3 = (x3 * qi) mod p
	mpz_addmul(r->x2, r->x3, r->q);			// x2 = x2 + x3 * q
	mpz_export(data, outlen, 1, 1, 0, 0, r->x2);	// data = export(x2)
}
