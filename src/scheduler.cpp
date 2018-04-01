#include <chrono>
#include <condition_variable>
#include <thread>

#include "scheduler.h"
#include "log.h"
#include "system.h"

#include "avltree.h"
#include "tree.h"

struct SchEntry{
	int64 id;
	int64 time;
	Task task;
};

int SchEntry_cmp(void *a, void *b){
	SchEntry *lhs = (SchEntry*)a;
	SchEntry *rhs = (SchEntry*)b;
	if(lhs->id == rhs->id)
		return 0;
	if(lhs->time < rhs->time)
		return -1;
	return 1;
}

// scheduler entries
#define SCH_MAX_ENTRIES 0x3FFF
static struct tree *entries = NULL;

// scheduler control
static std::thread thr;
static std::mutex mtx;
static std::condition_variable cond;
static int64 next_id = 1;
static bool running = false;

static void scheduler(void){
	int64			delta;
	Task			task;
	struct tree_node	*node;
	SchEntry		*entry;

	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		if(tree_empty(entries)){
			cond.wait(ulock);
			if(tree_empty(entries)){
				ulock.unlock();
				continue;
			}
		}

		node = tree_min(entries);
		entry = (SchEntry*)tree_node_key(node);
		delta = entry->time - sys_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			node = tree_min(entries);
			entry = (SchEntry*)tree_node_key(node);
			if(entry == NULL || entry->time > sys_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove entry
		task = std::move(entry->task);
		avl_remove(entries, node);
		ulock.unlock();

		dispatcher_add(std::move(task));

		// TODO: each task has a dispatcher
		//before unlocking:
		//disp = task->dispatcher;
		//after unlocking
		//if(disp != nullptr)
		//	dispatcher_add(disp, std::move(task));
		//else
		//	dispatcher_add(std::move(task));
	}
}

bool scheduler_init(void){
	// create entries tree
	entries = tree_create(SCH_MAX_ENTRIES,
		sizeof(SchEntry), SchEntry_cmp);
	if(entries == nullptr){
		LOG_ERROR("scheduler_init: failed to create entries tree");
		return false;
	}

	// spawn scheduler thread
	running = true;
	thr = std::thread(scheduler);
	return true;
}

void scheduler_shutdown(void){
	// join scheduler thread
	mtx.lock();
	running = false;
	cond.notify_one();
	mtx.unlock();
	thr.join();

	// release entries tasks
	struct tree_node *node = tree_min(entries);
	while(node != NULL){
		// release task
		((SchEntry*)tree_node_key(node))->task = nullptr;
		node = tree_node_next(node);
	}

	// release entries tree
	tree_destroy(entries);
}

SchRef scheduler_add(int64 delay, const Task &task){
	return scheduler_add(delay, Task(task));
}

SchRef scheduler_add(int64 delay, Task &&task){
	struct tree_node	*node;
	SchEntry		*entry;
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	node = tree_alloc_node(entries);
	if(node == nullptr){
		LOG("scheduler_add: scheduler is at maximum capacity (%d)", SCH_MAX_ENTRIES);
		return SCHREF_INVALID;
	}
	entry = (SchEntry*)tree_node_key(node);
	entry->id = next_id++;
	entry->time = time;
	entry->task = std::move(task);
	avl_insert_node(entries, node);
	if(node == tree_min(entries))
		cond.notify_one();
	return SchRef(entry->id, entry->time);
}

bool scheduler_remove(const SchRef &ref){
	struct tree_node *node;
	std::lock_guard<std::mutex> lock(mtx);
	node = tree_find_node(entries, (void*)&ref);
	if(node == NULL){
		LOG("scheduler_remove: trying to remove invalid entry");
		return false;
	}
	// release task
	((SchEntry*)tree_node_key(node))->task = nullptr;
	avl_remove(entries, node);
	return true;
}

bool scheduler_reschedule(int64 delay, SchRef &ref){
	struct tree_node	*old_node;
	struct tree_node	*new_node;
	SchEntry		*old;
	SchEntry		*entry;
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	old_node = tree_find(entries, (void*)&ref);
	if(old_node == NULL){
		LOG("scheduler_reschedule: trying to reschedule invalid entry");
		return false;
	}

	new_node = tree_alloc_node(entries);
	if(new_node == NULL){
		LOG("scheduler_reschedule: failed to re-insert entry"
			"(scheduler is at maximum capactity `%d`)", SCH_MAX_ENTRIES);
		return false;
	}

	old = (SchEntry*)tree_node_key(old_node);
	entry = (SchEntry*)tree_node_key(new_node);
	entry->id = next_id++;
	entry->time = time;
	entry->task = std::move(old->task);

	// remove old entry and update SchRef
	avl_remove(entries, old_node);
	ref.id = entry->id;
	ref.time = entry->time;

	if(new_node == tree_min(entries))
		cond.notify_one();
	return true;
}
