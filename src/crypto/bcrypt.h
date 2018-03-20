#ifndef CRYPTO_BCRYPT_H_
#define CRYPTO_BCRYPT_H_

#include "../def.h"

bool bcrypt_newhash(const char *pass, int logr, char *hash, size_t hashlen);
bool bcrypt_checkpass(const char *pass, const char *hash);

#endif //CRYPTO_BCRYPT_H_
