#define DB_INTERNAL 1
#include "database.h"
#include "../ringbuffer.h"
#include "../thread.h"

static bool running;
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;

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
			condvar_wait(&cv, &mtx);
			if(!running){
				mutex_unlock(&mtx);
				return NULL;
			}
		}
		// pop task
		next = RINGBUFFER_UNCHECKED_POP(dbtasks);
		fp = next->fp;
		arg = next->arg;
		mutex_unlock(&mtx);

		// execute task
		fp(arg);
	}
	//return NULL;
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
	condvar_init(&cv);
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
	condvar_broadcast(&cv);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	// close database connection
	db_internal_connection_close();
}

bool db_add_task(void (*fp)(void*), void *arg){
	struct task *task;
	mutex_lock(&mtx);
	if(RINGBUFFER_FULL(dbtasks)){
		mutex_unlock(&mtx);
		DEBUG_LOG("db_add_task: task ringbuffer is full");
		return false;
	}
	task = RINGBUFFER_UNCHECKED_PUSH(dbtasks);
	task->fp = fp;
	task->arg = arg;
	condvar_signal(&cv);
	mutex_unlock(&mtx);
	return true;
}
