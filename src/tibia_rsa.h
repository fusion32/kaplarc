#ifndef KAPLAR_TIBIA_RSA_H_
#define KAPLAR_TIBIA_RSA_H_ 1

#include "common.h"
bool tibia_rsa_init(void);
void tibia_rsa_shutdown(void);
bool tibia_rsa_encode(uint8 *data, size_t len, size_t *outlen);
bool tibia_rsa_decode(uint8 *data, size_t len, size_t *outlen);

#endif //KAPLAR_TIBIA_RSA_H_
