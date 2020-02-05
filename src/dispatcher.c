#if 0
#include "dispatcher.h"
#include "log.h"
#include "system.h"
#include "thread.h"

struct task{
	void (*func)(void*);
	void *arg;
};

#define MAX_TASKS 0x8000
#if !IS_POWER_OF_TWO(MAX_TASKS)
#	error MAX_TASKS should be a power of two
#endif
#define RB_MASK (MAX_TASKS - 1)
struct dispatcher{
	mutex_t mtx;
	uint32 rdpos;
	uint32 wrpos;
	struct task tasks[MAX_TASKS];
};

// dispatcher thread & sync
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;
static bool running = false;

static void dispatcher_run(struct dispatcher *d){
	void (*func)(void*);
	void *arg;
	struct task *next;
	while(1){
		mutex_lock(&d->mtx);
		if(d->rdpos == d->wrpos){
			mutex_unlock(&d->mtx);
			return;
		}
		next = &d->tasks[d->rdpos++ & RB_MASK];
		func = next->func;
		arg = next->arg;
		mutex_unlock(&d->mtx);
	}
}

static void *dispatcher_thread(void *unused){
	void (*func)(void*);
	void *arg;
	struct task *next;
	while(running){
		// retrieve task
		mutex_lock(&mtx);
		if(rdpos == wrpos){
			condvar_wait(&cv, &mtx);
			if(rdpos == wrpos){
				mutex_unlock(&mtx);
				continue;
			}
		}
		next = &tasks[rdpos++ & RB_MASK];
		func = next->func;
		arg = next->arg;
		mutex_unlock(&mtx);

		// run task
		func(arg);
	}
	return NULL;
}

bool dispatcher_init(void){
	int ret;
	running = true;
	mutex_init(&mtx);
	condvar_init(&cv);
	ret = thread_create(&thr, dispatcher_thread, NULL);
	if(ret != 0){
		LOG_ERROR("dispatcher_init: failed to"
			" spawn dispatcher thread");
		return false;
	}
	return true;
}

void dispatcher_shutdown(void){
	// join dispatcher thread
	mutex_lock(&mtx);
	running = false;
	condvar_signal(&cv);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	// release resources
	condvar_destroy(&cv);
	mutex_destroy(&mtx);
}

void dispatcher_add(void (*func)(void*), void *arg){
	struct task *task;
	mutex_lock(&mtx);
	if((wrpos - rdpos) >= MAX_TASKS){
		LOG_ERROR("dispatcher_add: task ring buffer is"
			" at maximum capacity (%d)", MAX_TASKS);
	}else{
		task = &tasks[wrpos++ & RB_MASK];
		task->func = func;
		task->arg = arg;
		condvar_signal(&cv);
	}
	mutex_unlock(&mtx);
}

#endif
