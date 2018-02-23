#ifndef CRYPTO_RSA_H_
#define CRYPTO_RSA_H_

#include <stddef.h>
struct RSA;
typedef struct RSA RSA;
void rsa_init(struct RSA **rsa);
void rsa_clear(struct RSA *rsa);
bool rsa_setkey(struct RSA *rsa, const char *p, const char *q, const char *e);
bool rsa_can_encode(struct RSA *rsa, size_t len);
size_t rsa_encoding_limit(struct RSA *rsa);
void rsa_encode(struct RSA *rsa, char *msg, size_t len, size_t *plen);
void rsa_decode(struct RSA *rsa, char *msg, size_t len, size_t *plen);

#endif //CRYPTO_RSA_H_
