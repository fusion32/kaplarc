#ifndef KAPLAR_CRYPTO_BCRYPT_H_
#define KAPLAR_CRYPTO_BCRYPT_H_ 1

#include "../common.h"

bool bcrypt_newhash(const char *pass, int logr, char *hash, size_t hashlen);
bool bcrypt_checkpass(const char *pass, const char *hash);

#endif //KAPLAR_CRYPTO_BCRYPT_H_
