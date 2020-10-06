#define DB_INTERNAL 1
#include "database.h"
#include "../task_rbuffer.h"
#include "../thread.h"

static thread_t thr;

#define MAX_DB_TASKS 1024
static struct task_rbuffer *dbtasks;

static void *db_thread(void *unused){
	while(task_rbuffer_run_one(dbtasks))
		continue;
	return NULL;
}

bool db_init(void){
	// connect to database
	if(!db_internal_connect()){
		LOG_ERROR("db_init: failed to initialize database connection");
		return false;
	}
	// init task ringbuffer
	dbtasks = task_rbuffer_create(MAX_DB_TASKS);
	// create db thread
	if(thread_init(&thr, db_thread, NULL) != 0){
		task_rbuffer_destroy(dbtasks);
		db_internal_connection_close();
		return false;
	}
	return true;
}

void db_shutdown(void){
	// this will make `task_rbuffer_run_one()` return false
	// effectively ending the database thread
	task_rbuffer_set_inactive(dbtasks);

	thread_join(&thr, NULL);

	db_internal_connection_close();
}

bool db_add_task(void (*fp)(void*), void *arg){
	return task_rbuffer_push(dbtasks, fp, arg);
}
