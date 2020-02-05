#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"

bool scheduler_init(void);
void scheduler_shutdown(void);
bool scheduler_add(int64 delay, void (*func)(void*), void *arg);
int64 scheduler_work(void);

#endif //SCHEDULER_H_
