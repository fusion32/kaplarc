#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include "def.h"

#define ENV_WORKER	0
#define ENV_SCHEDULER	1
#define ENV_SERVER	2

void env_set(int idx);
bool env_reset(int idx);
bool env_lock(int *idx, int n);
void env_unlock(int *idx, int n);

#endif //ENVIRONMENT_H_
