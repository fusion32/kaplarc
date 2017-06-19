#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "work.h"
#include <forward_list>

namespace kp{

struct sch_work{
	long time;
	kp::work wrk;
};

using sch_entry = std::forward_list<sch_work>::const_iterator;

} //namespace

void			scheduler_init(void);
void			scheduler_shutdown(void);
kp::sch_entry		scheduler_add(long delay, kp::work work_);
bool			scheduler_remove(kp::sch_entry &entry);
bool			scheduler_reschedule(long delay, kp::sch_entry &entry);
bool			scheduler_pop(kp::sch_entry &entry);

#endif //SCHEDULER_H_
