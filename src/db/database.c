#define DB_INTERNAL 1
#include "database.h"
#include "../ringbuffer.h"
#include "../thread.h"

static bool running;
static thread_t thr;
static mutex_t mtx;
static condvar_t rb_full;
static condvar_t rb_empty;
DECL_TASK_RINGBUFFER(dbtasks, 1024)

static void *db_thread(void *unused){
	struct task *next;
	void (*fp)(void*);
	void *arg;

	while(1){
		mutex_lock(&mtx);
		// check if still running
		if(!running){
			mutex_unlock(&mtx);
			return NULL;
		}
		// wait for task
		while(RINGBUFFER_EMPTY(dbtasks)){
			condvar_wait(&rb_empty, &mtx);
			if(!running){
				mutex_unlock(&mtx);
				return NULL;
			}
		}
		// signal any thread that might be waiting
		// for the ringbuffer to have room
		if(RINGBUFFER_FULL(dbtasks))
			condvar_signal(&rb_full);
		// pop task
		next = RINGBUFFER_UNCHECKED_POP(dbtasks);
		fp = next->fp;
		arg = next->arg;
		mutex_unlock(&mtx);

		// execute task
		fp(arg);
	}
	return NULL;
}

bool db_init(void){
	// connect to database
	if(!db_internal_connect()){
		LOG_ERROR("db_init: failed to initialize database connection");
		return false;
	}
	// create db thread
	running = true;
	mutex_init(&mtx);
	condvar_init(&rb_full);
	condvar_init(&rb_empty);
	if(thread_init(&thr, db_thread, NULL) != 0){
		mutex_destroy(&mtx);
		db_internal_connection_close();
		return false;
	}
	return true;
}

void db_shutdown(void){
	// join database thread
	mutex_lock(&mtx);
	running = false;
	condvar_broadcast(&rb_full);
	condvar_broadcast(&rb_empty);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	// close database connection
	db_internal_connection_close();
}

bool db_add_task(void (*fp)(void*), void *arg){
	struct task *task;
	mutex_lock(&mtx);
	if(!running){
		mutex_unlock(&mtx);
		return false;
	}
	// wait for ringbuffer to have room
	while(RINGBUFFER_FULL(dbtasks)){
		DEBUG_LOG("db_add_task: task ringbuffer is full");
		condvar_wait(&rb_full, &mtx);
		if(!running){
			mutex_unlock(&mtx);
			return false;
		}
	}
	// signal database thread that there's work to be done
	if(RINGBUFFER_EMPTY(dbtasks))
		condvar_signal(&rb_empty);
	task = RINGBUFFER_UNCHECKED_PUSH(dbtasks);
	task->fp = fp;
	task->arg = arg;
	mutex_unlock(&mtx);
	return true;
}
