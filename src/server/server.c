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
static condvar_t sync_cv;
static void (*sync_fp)(void*);
static void *sync_arg;

/* IMPL START */
void *server_thread(void *unused){
	while(1){
		// consume dispatched work
		mutex_lock(&mtx);
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		if(sync_fp != NULL){
			sync_fp(sync_arg);
			sync_fp = NULL;
			condvar_signal(&sync_cv);
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
	sync_fp = NULL;
	sync_arg = NULL;
	mutex_init(&mtx);
	condvar_init(&sync_cv);
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
	// sync_fp = NULL;
	condvar_broadcast(&sync_cv);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	condvar_destroy(&sync_cv);
	mutex_destroy(&mtx);
	server_internal_shutdown();
}

void server_sync(void (*fp)(void*), void *arg){
	mutex_lock(&mtx);
	if(!running){
		mutex_unlock(&mtx);
		return;
	}
	DEBUG_ASSERT(sync_fp == NULL);
	sync_fp = fp;
	sync_arg = arg;
	server_internal_interrupt();
	condvar_wait(&sync_cv, &mtx);
	DEBUG_ASSERT(sync_fp == NULL);
	mutex_unlock(&mtx);
}
