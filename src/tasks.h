#ifndef TASKS_H_
#define TASKS_H_

#include "def.h"

#define MAX_TASKS 0x8000
#if !IS_POWER_OF_TWO(MAX_TASKS)
#	error MAX_TASKS should be a power of two
#endif
#define TASKS_MASK (MAX_TASKS - 1)

struct task{
	void (*fp)(void*);
	void *arg;
};

#endif //TASKS_H_
