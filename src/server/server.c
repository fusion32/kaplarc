#include "../def.h"
#include "../tasks.h"
#include "../thread.h"
#include "server.h"

/* these will depend on the OS */
bool internal_server_init(void);
void internal_server_shutdown(void);
void internal_server_work(void);
void internal_server_interrupt(void);

/* server state and dispatcher */
static thread_t thr;
static mutex_t mtx;
static bool running;
static uint32 insert_idx;
static uint32 insert_pos;
static uint32 num_exec;
static struct task tasks[2][MAX_TASKS];

void *server_thread(void *unused){
	struct task *exec_buf;
	uint32 _num_exec, i;
	while(1){
		// consume dispatched work
		mutex_lock(&mtx);
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		exec_buf = &tasks[1 - insert_idx][0];
		_num_exec = num_exec;
		for(i = 0; i < _num_exec; i += 1)
			exec_buf[i].fp(exec_buf[i].arg);
		mutex_unlock(&mtx);

		// consume net i/o
		internal_server_work();
	}
	return NULL;
}

bool server_init(void){
	if(!internal_server_init())
		return false;
	running = true;
	insert_idx = 0;
	insert_pos = 0;
	num_exec = 0;
	mutex_init(&mtx);
	if(thread_create(&thr, server_thread, NULL) != 0){
		mutex_destroy(&mtx);
		internal_server_shutdown();
		return false;
	}
	return true;
}

void server_shutdown(void){
	mutex_lock(&mtx);
	running = false;
	internal_server_interrupt();
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);
	mutex_destroy(&mtx);
	internal_server_shutdown();
}

// @NOTE: this assumes a single thread will add tasks
// (the same thread that add tasks will also flush them)
bool server_add_task(void (*fp)(void*), void *arg){
	struct task *task;
	if(insert_pos >= MAX_TASKS)
		return false;
	task = &tasks[insert_idx][insert_pos++];
	task->fp = fp;
	task->arg = arg;
	return true;
}
bool server_add_tasks(struct task *arr, uint32 num_arr){
	uint32 updated_pos = insert_pos + num_arr;
	if(updated_pos >= MAX_TASKS)
		return false;
	memcpy(&tasks[insert_idx][insert_pos],
		arr, sizeof(struct task) * num_arr);
	insert_pos = updated_pos;
	return true;
}

void server_flush_tasks(void){
	mutex_lock(&mtx);
	insert_idx = 1 - insert_idx;
	num_exec = insert_pos;
	insert_pos = 0;
	internal_server_interrupt();
	mutex_unlock(&mtx);
}
