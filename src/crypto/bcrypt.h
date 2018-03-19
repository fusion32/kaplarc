#ifndef CRYPTO_BCRYPT_H_
#define CRYPTO_BCRYPT_H_

#include "../def.h"

bool bcrypt(const char *pass, int logr, char *hash, size_t hashlen);
bool bcrypt_check(const char *pass, const char *hash);

#endif //CRYPTO_BCRYPT_H_
