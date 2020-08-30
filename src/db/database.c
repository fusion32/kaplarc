#define DB_INTERNAL 1
#include "database.h"
#include "../thread.h"

static bool running;
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;

struct db_task{
	void (*fp)(void*, size_t);
	size_t lstore_size;
	uint8 lstore[DB_TASK_MAX_LOCAL_STORAGE];
};
#define MAX_QUEUED_TASKS 1024
#if !IS_POWER_OF_TWO(MAX_QUEUED_TASKS)
#	error "MAX_QUEUED_TASKS must be a power of two."
#endif
#define TASKS_BITMASK (MAX_QUEUED_TASKS - 1)

static struct db_task tasks[MAX_QUEUED_TASKS];
static uint32 readpos = 0;
static uint32 writepos = 0;

static INLINE bool tasks_empty(void){
	return readpos == writepos;
}
static INLINE bool tasks_full(void){
	return (writepos - readpos) >= MAX_QUEUED_TASKS;
}
// NOTE: push and pop don't check if the ring is full or empty
static INLINE void tasks_push(void (*fp)(void*), void *lstore, size_t lstore_size){
	uint32 idx = writepos++ & TASKS_BITMASK;
	tasks[idx].fp = fp;
	tasks[idx].lstore_size = lstore_size;
	if(lstore != NULL)
		memcpy(tasks[idx].lstore, lstore, lstore_size);
}
static INLINE struct db_task *tasks_pop(void){
	return &tasks[readpos++ & TASKS_BITMASK];
}

static void *db_thread(void *unused){
	struct db_task *next;
	void (*fp)(void*);
	size_t lstore_size;
	uint8 lstore[DB_TASK_MAX_LOCAL_STORAGE];

	while(1){
		mutex_lock(&mtx);
		// check if still running
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		// wait for task
		if(tasks_empty()){
			condvar_wait(&cv, &mtx);
			if(!running || tasks_empty()){
				mutex_unlock(&mtx);
				continue;
			}
		}
		// pop task
		next = tasks_pop();
		fp = next->fp;
		lstore_size = next->lstore_size;
		memcpy(lstore, next->lstore, lstore_size);
		mutex_unlock(&mtx);

		// execute task
		fp(lstore, lstore_size);
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

bool db_add_task(void (*fp)(void*, size_t), void *lstore, size_t lstore_size){
	mutex_lock(&mtx);
	if(tasks_full() || lstore_size > DB_TASK_MAX_LOCAL_STORAGE){
		mutex_unlock(&mtx);
		if(tasks_full())
			DEBUG_LOG("db_add_task: task ringbuffer is full");
		if(lstore_size > DB_TASK_MAX_LOCAL_STORAGE)
			DEBUG_LOG("db_add_task: lstore_size is limited to %d",
				DB_TASK_MAX_LOCAL_STORAGE);
		return false;
	}
	tasks_push(fp, lstore, lstore_size);
	mutex_unlock(&mtx);
	return true;
}
