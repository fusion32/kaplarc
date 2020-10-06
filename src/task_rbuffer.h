#ifndef KAPLAR_TASK_RBUFFER_H_
#define KAPLAR_TASK_RBUFFER_H_ 1

#include "common.h"

struct task{
	void (*fp)(void*);
	void *arg;
};

struct task_rbuffer;
struct task_rbuffer *task_rbuffer_create(uint32 max_tasks);
void task_rbuffer_destroy(struct task_rbuffer *rb);
bool task_rbuffer_push(struct task_rbuffer *rb, void (*fp)(void*), void *arg);
bool task_rbuffer_run_one(struct task_rbuffer *rb);
void task_rbuffer_set_inactive(struct task_rbuffer *rb);


#endif //KAPLAR_TASK_RBUFFER_H_
