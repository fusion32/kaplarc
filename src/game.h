#ifndef KAPLAR_GAME_H_
#define KAPLAR_GAME_H_ 1

#include "common.h"

bool game_add_task(void (*fp)(void*), void *arg);
bool game_add_server_task(void (*fp)(void*), void *arg);

bool game_init(void);
void game_shutdown(void);
void game_run(void);


#endif //KAPLAR_GAME_H
