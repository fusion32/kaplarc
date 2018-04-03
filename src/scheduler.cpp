#include <chrono>
#include <condition_variable>
#include <thread>

#include "scheduler.h"
#include "log.h"
#include "system.h"
#include "avltree.hpp"

struct SchEntry : public SchRef{
	Task task;

	SchEntry(void) : SchRef() {}
	SchEntry(int64 id_, int64 time_, const Task &task_)
		: SchRef(id_, time_), task(task_) {}
	SchEntry(int64 id_, int64 time_, Task &&task_)
		: SchRef(id_, time_), task(std::move(task_)) {}
};

// scheduler entries
#define SCH_MAX_ENTRIES 0x3FFF
static AVLTree<SchEntry, std::less<SchRef>> entries;

// scheduler control
static std::thread thr;
static std::mutex mtx;
static std::condition_variable cond;
static int64 next_id = 1;
static bool running = false;

static void scheduler(void){
	int64			delta;
	Task			task;
	TreeIterator<SchEntry>	entry;

	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		if(entries.empty()){
			cond.wait(ulock);
			if(entries.empty()){
				ulock.unlock();
				continue;
			}
		}

		entry = entries.min();
		delta = entry->time - sys_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			entry = entries.min();
			if(entry == entries.end() || entry->time > sys_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove entry
		task = std::move(entry->task);
		entries.remove(entry);
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
	if(!entries.create(SCH_MAX_ENTRIES)){
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

	// release entries
	entries.destroy();
}

SchRef scheduler_add(int64 delay, const Task &task){
	return scheduler_add(delay, Task(task));
}

SchRef scheduler_add(int64 delay, Task &&task){
	TreeIterator<SchEntry> entry;
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	entry = entries.insert(next_id++, time, std::move(task));
	if(entry == entries.end()){
		LOG("scheduler_add: scheduler is at maximum capacity (%d)", SCH_MAX_ENTRIES);
		return SCHREF_INVALID;
	}
	if(entry == entries.min())
		cond.notify_one();
	return SchRef(entry->id, entry->time);
}

bool scheduler_remove(const SchRef &ref){
	TreeIterator<SchEntry> entry;
	std::lock_guard<std::mutex> lock(mtx);
	entry = entries.find(ref);
	if(entry == entries.end()){
		LOG("scheduler_remove: trying to remove invalid entry");
		return false;
	}
	entries.remove(entry);
	return true;
}

bool scheduler_reschedule(int64 delay, SchRef &ref){
	TreeIterator<SchEntry> old, entry;
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	old = entries.find(ref);
	if(old == entries.end()){
		LOG("scheduler_reschedule: trying to reschedule invalid entry");
		return false;
	}
	entry = entries.insert(next_id++, time, std::move(old->task));
	if(entry == entries.end()){
		LOG("scheduler_reschedule: failed to re-insert entry"
			"(scheduler is at maximum capactity `%d`)", SCH_MAX_ENTRIES);
		return false;
	}

	// remove old entry and update SchRef
	entries.remove(old);
	ref.id = entry->id;
	ref.time = entry->time;

	if(entry == entries.min())
		cond.notify_one();
	return true;
}
