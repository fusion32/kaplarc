#if 0
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
static mutex_t mtx;
static int64 next_id = 1;

bool scheduler_init(void){
	rbt_init(&tree, schnode_cmp);
	slab = slab_create(MAX_NODES, sizeof(struct schnode));
	if(slab == NULL){
		LOG_ERROR("scheduler_init: failed to create node pool");
		return false;
	}
	mutex_init(&mtx);
	return true;
}

void scheduler_shutdown(void){
	mutex_destroy(&mtx);
	slab_destroy(slab);
}

bool scheduler_add(int64 delay, void (*func)(void*), void *arg){
	struct schnode *node;
	int64 time = delay + sys_tick_count();
	mutex_lock(&mtx);
	node = slab_alloc(slab);
	if(node == NULL){
		LOG_ERROR("scheduler_add: reached maximum"
			" capacity (%d)", MAX_NODES);
		mutex_unlock(&mtx);
		return false;
	}
	node->id = next_id++;
	node->time = time;
	node->func = func;
	node->arg = arg;
	ASSERT(SCH_INSERT(&tree, node));
	mutex_unlock(&mtx);
	return true;
}

int64 scheduler_work(void){
	void (*func)(void*);
	void *arg;
	struct schnode *next;
	int64 now;
	while(1){
		now = sys_tick_count();
		mutex_lock(&mtx);
		next = SCH_MIN(&tree);
		if(next == NULL){
			mutex_unlock(&mtx);
			return -1;
		}
		if(next->time > now){
			mutex_unlock(&mtx);
			return next->time;
		}
		func = next->func;
		arg = next->arg;
		SCH_REMOVE(&tree, next);
		slab_free(slab, next);
		mutex_unlock(&mtx);

		// dispatch task
		dispatcher_add(func, arg);
	}
	return -1;
}

#endif
