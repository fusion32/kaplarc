#include "dispatcher.h"
#include "log.h"
#include "system.h"
#include "thread.h"

struct task{
	void (*func)(void*);
	void *arg;
};

// dispatcher ringbuffer
#define MAX_TASKS 0x8000
static struct task tasks[MAX_TASKS];
static uint32 rdpos = 0;
static uint32 wrpos = 0;

#if !IS_POWER_OF_TWO(MAX_TASKS)
#	error MAX_TASKS must be a power of two
#endif
#define RB_MASK (MAX_TASKS - 1)

// dispatcher thread & sync
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;
static bool running = false;

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
