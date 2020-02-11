#ifndef GAME_GAME_H_
#define GAME_GAME_H_

#include "def.h"

// tibia protocols
extern struct protocol protocol_login;
//extern struct protocol protocol_game;
//extern struct protocol protocol_old_login;
//extern struct protocol protocol_old_game;
//extern struct protocol protocol_info;

bool game_init(void);
void game_shutdown(void);
void game_run(void);

//
void game_dispatch(void);
bool game_schedule(void);

#endif //GAME_GAME_H
