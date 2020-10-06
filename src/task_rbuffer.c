#include "task_rbuffer.h"
#include "thread.h"

struct task_rbuffer{
	mutex_t lock;
	condvar_t rb_full;
	condvar_t rb_empty;
	bool active;
	uint32 index_mask;
	uint32 readpos;
	uint32 writepos;
	struct task buffer[];
};

struct task_rbuffer *task_rbuffer_create(uint32 max_tasks){
	ASSERT(IS_POWER_OF_TWO(max_tasks));
	struct task_rbuffer *rb = kpl_malloc(
		sizeof(struct task_rbuffer) +
		sizeof(struct task) * max_tasks);
	mutex_init(&rb->lock);
	condvar_init(&rb->rb_full);
	condvar_init(&rb->rb_empty);
	rb->active = true;
	rb->index_mask = max_tasks - 1;
	rb->readpos = 0;
	rb->writepos = 0;
	return rb;
}

void task_rbuffer_destroy(struct task_rbuffer *rb){
	mutex_destroy(&rb->lock);
	condvar_destroy(&rb->rb_full);
	condvar_destroy(&rb->rb_empty);
	kpl_free(rb);
}

static INLINE bool task_rbuffer_empty(struct task_rbuffer *rb){
	return rb->readpos == rb->writepos;
}

static INLINE bool task_rbuffer_full(struct task_rbuffer *rb){
	return (rb->writepos - rb->readpos) > rb->index_mask;
}

bool task_rbuffer_push(struct task_rbuffer *rb, void (*fp)(void*), void *arg){
	struct task *task;
	mutex_lock(&rb->lock);
	if(!rb->active){
		mutex_unlock(&rb->lock);
		return false;
	}
	if(task_rbuffer_full(rb)){
		// wait until there is room in the ringbuffer
		do{
			condvar_wait(&rb->rb_full, &rb->lock);
		}while(task_rbuffer_full(rb) && rb->active);
		if(!rb->active){
			mutex_unlock(&rb->lock);
			return false;
		}
	}else if(task_rbuffer_empty(rb)){
		// signal the worker thread that there is a new task
		condvar_signal(&rb->rb_empty);
	}
	task = &rb->buffer[rb->writepos++ & rb->index_mask];
	task->fp = fp;
	task->arg = arg;
	mutex_unlock(&rb->lock);
	return true;
}

bool task_rbuffer_run_one(struct task_rbuffer *rb){
	struct task *task;
	void (*fp)(void*);
	void *arg;
	mutex_lock(&rb->lock);
	if(!rb->active){
		mutex_unlock(&rb->lock);
		return false;
	}
	if(task_rbuffer_empty(rb)){
		// wait until theres a task to be executed
		do{
			condvar_wait(&rb->rb_empty, &rb->lock);
		}while(task_rbuffer_empty(rb) && rb->active);
		if(!rb->active){
			mutex_unlock(&rb->lock);
			return false;
		}
	}else if(task_rbuffer_full(rb)){
		// signal any thread waiting for there to be room in the ringbuffer
		condvar_signal(&rb->rb_full);
	}
	task = &rb->buffer[rb->readpos++ & rb->index_mask];
	fp = task->fp;
	arg = task->arg;
	mutex_unlock(&rb->lock);

	fp(arg);
	return true;
}

void task_rbuffer_set_inactive(struct task_rbuffer *rb){
	mutex_lock(&rb->lock);
	rb->active = false;
	condvar_broadcast(&rb->rb_full);
	condvar_broadcast(&rb->rb_empty);
	mutex_unlock(&rb->lock);
}
