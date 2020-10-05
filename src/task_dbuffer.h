#ifndef KAPLAR_TASK_DBUFFER_H_
#define KAPLAR_TASK_DBUFFER_H_ 1

#include "common.h"
#include "thread.h"

struct task{
	void (*fp)(void*);
	void *arg;
};

struct task_dbuffer;
struct task_dbuffer *task_dbuffer_create(uint32 max_tasks);
void task_dbuffer_destroy(struct task_dbuffer *db);
bool task_dbuffer_add(struct task_dbuffer *db, void (*fp)(void*), void *arg);
void task_dbuffer_swap_and_run(struct task_dbuffer *db);
void task_dbuffer_wake_all(struct task_dbuffer *db);

#endif //KAPLAR_TASK_DBUFFER_H_
