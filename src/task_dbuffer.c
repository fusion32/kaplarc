#include "task_dbuffer.h"

struct task_dbuffer{
	mutex_t write_lock;
	condvar_t buffer_full;
	bool active;
	int write_idx;
	uint32 max_tasks;
	uint32 count[2];
	struct task *buffer[2];
};

struct task_dbuffer *task_dbuffer_create(uint32 max_tasks){
	struct task_dbuffer *db = kpl_malloc(sizeof(struct task_dbuffer));
	mutex_init(&db->write_lock);
	condvar_init(&db->buffer_full);
	db->active = true;
	db->write_idx = 0;
	db->max_tasks = max_tasks;
	db->count[0] = 0;
	db->count[1] = 0;
	db->buffer[0] = kpl_malloc(sizeof(struct task) * max_tasks * 2);
	db->buffer[1] = db->buffer[0] + max_tasks;
	return db;
}

void task_dbuffer_destroy(struct task_dbuffer *db){
	mutex_destroy(&db->write_lock);
	condvar_destroy(&db->buffer_full);
	kpl_free(db->buffer[0]);
	kpl_free(db);
}

bool task_dbuffer_add(struct task_dbuffer *db, void (*fp)(void*), void *arg){
	struct task *task;
	uint32 count;
	int idx;

	mutex_lock(&db->write_lock);
	idx = db->write_idx;
	count = db->count[idx];
	while(count >= db->max_tasks && db->active){
		condvar_wait(&db->buffer_full, &db->write_lock);
		count = db->count[idx];
	}
	if(!db->active){
		mutex_unlock(&db->write_lock);
		return false;
	}
	task = &db->buffer[idx][count];
	task->fp = fp;
	task->arg = arg;
	db->count[idx] += 1;
	mutex_unlock(&db->write_lock);
	return true;
}

void task_dbuffer_swap_and_run(struct task_dbuffer *db){
	struct task *task;
	uint32 count;
	int idx;

	mutex_lock(&db->write_lock);
	// get write buffer info
	idx = db->write_idx;
	count = db->count[idx];
	task = &db->buffer[idx][0];
	// swap buffers
	db->write_idx = 1 - db->write_idx;
	db->count[db->write_idx] = 0;
	// signal any thread that may be waiting for
	// a free buffer slot
	if(count >= db->max_tasks)
		condvar_broadcast(&db->buffer_full);
	mutex_unlock(&db->write_lock);

	// run queued tasks (this is OK because `swap_and_run`
	// should only be called from a single thread!)
	while(count > 0){
		task->fp(task->arg);
		task += 1;
		count -= 1;
	}
}

void task_dbuffer_set_inactive(struct task_dbuffer *db){
	mutex_lock(&db->write_lock);
	db->active = false;
	condvar_broadcast(&db->buffer_full);
	mutex_unlock(&db->write_lock);
}
