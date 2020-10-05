#include "server.h"
#include "../thread.h"

/* these will depend on the OS */
bool server_internal_init(void);
void server_internal_shutdown(void);
void server_internal_work(void);
void server_internal_interrupt(void);

/* server thread control */
static thread_t thr;
static mutex_t mtx;
static bool running;
static void (*task_fp)(void*);
static void *task_arg;

/* IMPL START */
void *server_thread(void *unused){
	void (*fp)(void*);
	void *arg;

	while(1){
		mutex_lock(&mtx);
		// check if still running
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		// we need to read these values inside the lock
		fp = task_fp;
		arg = task_arg;
		task_fp = NULL;
		mutex_unlock(&mtx);

		// execute server task
		if(fp != NULL)
			fp(arg);

		// consume net i/o
		server_internal_work();
	}
	return NULL;
}

bool server_init(void){
	if(!server_internal_init())
		return false;
	running = true;
	task_fp = NULL;
	mutex_init(&mtx);
	if(thread_init(&thr, server_thread, NULL) != 0){
		mutex_destroy(&mtx);
		server_internal_shutdown();
		return false;
	}
	return true;
}

void server_shutdown(void){
	mutex_lock(&mtx);
	running = false;
	server_internal_interrupt();
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	mutex_destroy(&mtx);
	server_internal_shutdown();
}

void server_exec(void (*fp)(void*), void *arg){
	mutex_lock(&mtx);
	DEBUG_ASSERT(task_fp == NULL);
	if(!running){
		mutex_unlock(&mtx);
		return;
	}
	task_fp = fp;
	task_arg = arg;
	server_internal_interrupt();
	mutex_unlock(&mtx);
}
