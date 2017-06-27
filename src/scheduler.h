#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"
#include "work.h"

namespace kp{
struct schref{
	int64 id;
	int64 time;
};
} // namespace

void		scheduler_init(void);
void		scheduler_shutdown(void);
kp::schref	scheduler_add(int64 delay, kp::work work_);
bool		scheduler_remove(const kp::schref &ref);

#endif //SCHEDULER_H_
