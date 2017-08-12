#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"
#include "work.h"

#define SCHREF_INVALID {-1, 0}

struct SchRef{
	int64 id;
	int64 time;
};

void	scheduler_init(void);
void	scheduler_shutdown(void);
SchRef	scheduler_add(int64 delay, Work wrk);
bool	scheduler_remove(const SchRef &ref);

#endif //SCHEDULER_H_
