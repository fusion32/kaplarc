#ifndef GAME_RSA_H_
#define GAME_RSA_H_

#include "../def.h"
bool grsa_init(void);
void grsa_shutdown(void);
bool grsa_encode(uint8 *data, size_t len, size_t *plen);
bool grsa_decode(uint8 *data, size_t len, size_t *plen);

#endif //GAME_RSA_H_
