#ifndef CRYPTO_ARC4RANDOM_H_
#define CRYPTO_ARC4RANDOM_H_

#include "../platform.h"

#ifdef PLATFORM_BSD
#include <stdlib.h>
#else
#include "../def.h"
uint32 arc4random(void);
void arc4random_buf(void *data, size_t len);
#endif

#endif //CRYPTO_ARC4RANDOM_H_