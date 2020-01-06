#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"

struct schnode;

bool	scheduler_init(void);
void	scheduler_shutdown(void);
struct schnode *scheduler_add(int64 delay, void (*func)(void*), void *arg);
bool	scheduler_remove(struct schnode *node);
bool	scheduler_reschedule(struct schnode *node, int64 delay);

#endif //SCHEDULER_H_
