#ifndef KAPLAR_CRYPTO_BASE64_H_
#define KAPLAR_CRYPTO_BASE64_H_ 1

#include "../common.h"

// turn len bytes of data into base64 data
bool base64_encode(char *b64data, const uint8 *data, size_t len);

// read len (after decoding) bytes of data from base64 data
bool base64_decode(uint8 *data, size_t len, const char *b64data);

#endif //KAPLAR_CRYPTO_BASE64_H_
