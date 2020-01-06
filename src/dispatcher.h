#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "def.h"

bool dispatcher_init(void);
void dispatcher_shutdown(void);
void dispatcher_add(void (*func)(void*), void *arg);

#endif //DISPATCHER_H_
