#ifndef SERVER_RSA_H_
#define SERVER_RSA_H_

#include "../def.h"
bool server_rsa_init(void);
void server_rsa_shutdown(void);
bool server_rsa_encode(uint8 *data, size_t len, size_t *plen);
bool server_rsa_decode(uint8 *data, size_t len, size_t *plen);

#endif //SERVER_RSA_H_
