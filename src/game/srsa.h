#ifndef GAME_SRSA_H_
#define GAME_SRSA_H_

#include "../def.h"
bool srsa_init(void);
void srsa_shutdown(void);
bool srsa_encode(char *msg, size_t len, size_t *plen);
bool srsa_decode(char *msg, size_t len, size_t *plen);

#endif //GAME_SRSA_H_
