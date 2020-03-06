#include "server.h"
#include "../def.h"
#include "../thread.h"

/* these will depend on the OS */
bool server_internal_init(void);
void server_internal_shutdown(void);
void server_internal_work(void);
void server_internal_interrupt(void);

/* server state and dispatcher */
static thread_t thr;
static mutex_t mtx;
static bool running;
static void (*exec_fp)(void*);
static void *exec_arg;

void *server_thread(void *unused){
	while(1){
		// consume dispatched work
		mutex_lock(&mtx);
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		if(exec_fp != NULL){
			exec_fp(exec_arg);
			exec_fp = NULL;
		}
		mutex_unlock(&mtx);

		// consume net i/o
		server_internal_work();
	}
	return NULL;
}

bool server_init(void){
	if(!server_internal_init())
		return false;
	running = true;
	exec_fp = NULL;
	exec_arg = NULL;
	mutex_init(&mtx);
	if(thread_create(&thr, server_thread, NULL) == 0)
		return true;

	mutex_destroy(&mtx);
	server_internal_shutdown();
	return false;
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

// @NOTE: This is called to run a task on the server thread
// and should be called once per game frame! I didn't add
// checks purposely to avoid branching on the fast path.
void server_exec(void (*fp)(void*), void *arg){
	mutex_lock(&mtx);
	DEBUG_ASSERT(exec_fp == NULL);
	exec_fp = fp;
	exec_arg = arg;
	server_internal_interrupt();
	mutex_unlock(&mtx);
}

#if 0
bool server_exec_check(void (*fp)(void*), void *arg){
	mutex_lock(&mtx);
	if(exec_fp != NULL){
		mutex_unlock(&mtx);
		return false;
	}
	exec_fp = fp;
	exec_arg = arg;
	server_internal_interrupt();
	mutex_unlock(&mtx);
	return true;
}
#endif
