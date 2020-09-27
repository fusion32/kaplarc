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
static void (*sv_sync_fp)(void*);
static void *sv_sync_arg;
static void (*sv_maintenance_fp)(void*);
static void *sv_maintenance_arg;

/* IMPL START */
void *server_thread(void *unused){
	void (*maintenance_fp)(void*);
	void *maintenance_arg;

	while(1){
		mutex_lock(&mtx);
		// check if still running
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		// execute sync function
		if(sv_sync_fp != NULL){
			sv_sync_fp(sv_sync_arg);
			sv_sync_fp = NULL;
			condvar_signal(&sync_cv);
		}
		// we need to read these values inside the lock
		maintenance_fp = sv_maintenance_fp;
		maintenance_arg = sv_maintenance_arg;
		sv_maintenance_fp = NULL;
		mutex_unlock(&mtx);

		// execute server task
		if(maintenance_fp != NULL)
			maintenance_fp(sv_maintenance_arg);

		// consume net i/o
		server_internal_work();
	}
	return NULL;
}

bool server_init(void){
	if(!server_internal_init())
		return false;
	running = true;
	sv_sync_fp = NULL;
	sv_maintenance_fp = NULL;
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
	condvar_broadcast(&sync_cv);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	condvar_destroy(&sync_cv);
	mutex_destroy(&mtx);
	server_internal_shutdown();
}

void server_sync(void (*sync_fp)(void*), void *sync_arg,
		void (*maintenance_fp)(void*), void *maintenance_arg){
	mutex_lock(&mtx);
	DEBUG_ASSERT(sv_sync_fp == NULL);
	if(!running){
		mutex_unlock(&mtx);
		return;
	}
	sv_sync_fp = sync_fp;
	sv_sync_arg = sync_arg;
	sv_maintenance_fp = maintenance_fp;
	sv_maintenance_arg = maintenance_arg;
	server_internal_interrupt();
	condvar_wait(&sync_cv, &mtx);
	mutex_unlock(&mtx);
}
