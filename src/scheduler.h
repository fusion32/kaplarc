#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"

struct schref{
	int64 id;
	int64 time;
};

bool scheduler_init(void);
void scheduler_shutdown(void);
bool scheduler_add(struct schref *outref, int64 delay,
		void (*func)(void*), void *arg);
bool scheduler_remove(struct schref *ref);
bool scheduler_reschedule(struct schref *ref, int64 delay);

#endif //SCHEDULER_H_
