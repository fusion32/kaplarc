#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"
#include "work.h"
#include <list>

namespace kp{

// define sch_entry as an opaque type
struct sch_entry;

} //namespace

void		scheduler_init(void);
void		scheduler_shutdown(void);
kp::sch_entry	*scheduler_add(long delay, kp::work work_);
bool		scheduler_remove(kp::sch_entry *entry);
bool		scheduler_reschedule(long delay, kp::sch_entry *entry);
bool		scheduler_pop(kp::sch_entry *entry);

#endif //SCHEDULER_H_
