#include "scheduler.h"
#include "dispatcher.h"
#include "log.h"
#include "slab.h"
#include "rbtree.h"
#include "system.h"
#include "thread.h"

struct schnode{
	struct rbnode n;
	int64 id;
	int64 time;
	void (*func)(void*);
	void *arg;
};

int schnode_cmp(struct rbnode *lhs, struct rbnode *rhs){
	struct schnode *l = (struct schnode*)lhs;
	struct schnode *r = (struct schnode*)rhs;
	if(l->id == r->id) return 0;
	else if(l->time < r->time) return -1;
	else return 1;
}

// it's possible to use the functions directly but there's
// a compatibility warning
#define SCH_MIN(t)		RBT_MIN(struct schnode, (t))
#define SCH_FIND(t, n)		RBT_FIND(struct schnode, (t), (n))
#define SCH_INSERT		RBT_INSERT
#define SCH_REMOVE		RBT_REMOVE

// scheduler tree
#define MAX_NODES 128
static struct rbtree tree;
static struct slab *slab;

// scheduler control
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;
static int64 next_id = 1;
static bool running = false;

static void *scheduler(void *unused){
	int64 delta;
	void (*func)(void*);
	void *arg;
	struct schnode *next;

	while(running){
		mutex_lock(&mtx);
		if(rbt_empty(&tree)){
			condvar_wait(&cv, &mtx);
			if(rbt_empty(&tree)){
				mutex_unlock(&mtx);
				continue;
			}
		}
		next = SCH_MIN(&tree);
		delta = next->time - sys_tick_count();
		if(delta > 0){
			condvar_timedwait(&cv, &mtx, delta);
			next = SCH_MIN(&tree);
			if(next == NULL || next->time > sys_tick_count()){
				mutex_unlock(&mtx);
				continue;
			}
		}
		func = next->func;
		arg = next->arg;
		SCH_REMOVE(&tree, next);
		slab_free(slab, next);
		mutex_unlock(&mtx);

		// dispatch task
		dispatcher_add(func, arg);
	}
	return NULL;
}

bool scheduler_init(void){
	// initialize tree
	rbt_init(&tree, schnode_cmp);

	// initialize node pool
	slab = slab_create(MAX_NODES, sizeof(struct schnode));
	if(slab == NULL){
		LOG_ERROR("scheduler_init: failed to create node pool");
		return false;
	}

	// spawn scheduler thread
	mutex_init(&mtx);
	condvar_init(&cv);
	running = true;
	if(thread_create(&thr, scheduler, NULL) != 0){
		LOG_ERROR("scheduler_init:"
			"failed to spawn scheduler thread");
		return false;
	}
	return true;
}

void scheduler_shutdown(void){
	// join scheduler thread
	mutex_lock(&mtx);
	running = false;
	condvar_signal(&cv);
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	// destroy all
	condvar_destroy(&cv);
	mutex_destroy(&mtx);
	slab_destroy(slab);
}

//@TODO: returning a pointer to the node is a bad idea because if
// we try to remove or reschedule the event after it went off we
// could end up messing up with another entry
struct schnode *scheduler_add(int64 delay, void (*func)(void*), void *arg){
	struct schnode *node;
	int64 time = delay + sys_tick_count();
	mutex_lock(&mtx);
	node = slab_alloc(slab);
	if(node == NULL){
		LOG_ERROR("scheduler_add: reached maximum"
			" capacity (%d)", MAX_NODES);
		mutex_unlock(&mtx);
		return NULL;
	}
	node->id = next_id++;
	node->time = time;
	node->func = func;
	node->arg = arg;
	ASSERT(SCH_INSERT(&tree, node));
	mutex_unlock(&mtx);
	return node;
}

bool scheduler_remove(struct schnode *node){
	mutex_lock(&mtx);
	node = SCH_FIND(&tree, node);
	if(node == NULL){
		LOG_WARNING("scheduler_remove:"
			" trying to remove invalid entry");
		mutex_unlock(&mtx);
		return false;
	}
	SCH_REMOVE(&tree, node);
	slab_free(slab, node);
	mutex_unlock(&mtx);
	return true;
}

bool scheduler_reschedule(struct schnode *node, int64 delay){
	int64 time = delay + sys_tick_count();
	mutex_lock(&mtx);
	node = SCH_FIND(&tree, node);
	if(node == NULL){
		LOG_WARNING("scheduler_reschedule:"
			" trying to reschedule invalid entry");
		return false;
	}
	SCH_REMOVE(&tree, node);
	node->time = time;
	SCH_INSERT(&tree, node);
	if(node == SCH_MIN(&tree))
		condvar_signal(&cv);
	return true;
}
