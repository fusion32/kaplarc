#include "task_rbuffer.h"
#include "thread.h"

struct task_rbuffer{
	mutex_t lock;
	condvar_t rb_full;
	condvar_t rb_empty;

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
	if(task_rbuffer_empty(rb)){
		condvar_signal(&rb->rb_empty);
	}else if(task_rbuffer_full(rb)){
		condvar_wait(&rb->rb_full, &rb->lock);
		if(task_rbuffer_full(rb)){
			mutex_unlock(&rb->lock);
			return false;
		}
	}
	task = &rb->buffer[rb->writepos++ & rb->index_mask];
	task->fp = fp;
	task->arg = arg;
	mutex_unlock(&rb->lock);
	return true;
}

bool task_rbuffer_pop(struct task_rbuffer *rb, struct task *out_task){
	struct task *task;
	mutex_lock(&rb->lock);
	if(task_rbuffer_full(rb)){
		condvar_signal(&rb->rb_full);
	}else if(task_rbuffer_empty(rb)){
		condvar_wait(&rb->rb_empty, &rb->lock);
		if(task_rbuffer_empty(rb)){
			mutex_unlock(&rb->lock);
			return false;
		}
	}
	task = &rb->buffer[rb->readpos++ & rb->index_mask];
	out_task->fp = task->fp;
	out_task->arg = task->arg;
	mutex_unlock(&rb->lock);
	return true;
}

void task_rbuffer_wake_all(struct task_rbuffer *rb){
	mutex_lock(&rb->lock);
	condvar_broadcast(&rb->rb_full);
	condvar_broadcast(&rb->rb_empty);
	mutex_unlock(&rb->lock);
}
