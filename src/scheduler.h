#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "def.h"
#include "dispatcher.h"

#define SCHREF_INVALID (-1)

struct SchRef{
	int64 id;
	int64 time;

	SchRef(int64 id_, int64 time_ = 0)
	  : id(id_), time(time_) {}
	SchRef &operator=(const int64 &rhs){
		id = rhs;
		return *this;
	}
	bool operator==(const int64 &rhs) const{
		return id == rhs;
	}
	bool operator!=(const int64 &rhs) const{
		return id != rhs;
	}
	bool operator==(const SchRef &rhs) const{
		return id == rhs.id;
	}
	bool operator!=(const SchRef &rhs) const{
		return id != rhs.id;
	}
};

bool	scheduler_init(void);
void	scheduler_shutdown(void);
SchRef	scheduler_add(int64 delay, const Task &task);
SchRef	scheduler_add(int64 delay, Task &&task);
bool	scheduler_remove(const SchRef &ref);
bool	scheduler_reschedule(int64 delay, SchRef &ref);

#endif //SCHEDULER_H_
