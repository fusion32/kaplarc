#include "dispatcher.h"
#include "log.h"
#include "scheduler.h"
#include "system.h"

#include <chrono>
#include <condition_variable>
#include <set>
#include <thread>

struct SchEntry : public SchRef{
	Dispatcher	*disp;
	Task		task;

	SchEntry(void) : SchRef() {}
	SchEntry(int64 id_, int64 time_, Dispatcher *disp_, const Task &task_)
		: SchRef(id_, time_), disp(disp_), task(task_) {}
	SchEntry(int64 id_, int64 time_, Dispatcher *disp_, Task &&task_)
		: SchRef(id_, time_), disp(disp_), task(std::move(task_)) {}
};

// comparisson needed by std::less
bool operator<(const SchRef &lhs, const SchRef &rhs){
	return (lhs.time == rhs.time) ?
		lhs.id < rhs.id :
		lhs.time < rhs.time;
}

// scheduler entries
static std::set<SchEntry, std::less<void>> entries;

// scheduler control
static std::thread thr;
static std::mutex mtx;
static std::condition_variable cond;
static int64 next_id = 1;
static bool running = false;

static void scheduler(void){
	int64		delta;
	Dispatcher	*disp;
	Task		task;
	auto		entry = entries.end();

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

		entry = entries.begin();
		delta = entry->time - sys_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			entry = entries.begin();
			if(entry == entries.end() || entry->time > sys_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove entry
		disp = entry->disp;
		task = std::move(entry->task);
		entries.erase(entry);
		ulock.unlock();

		// dispatch/execute task
		if(disp != nullptr){
			dispatcher_add(disp, std::move(task));
		}else{
			task();
		}
	}
}

void scheduler_init(void){
	// clear entries
	entries.clear();

	// spawn scheduler thread
	running = true;
	thr = std::thread(scheduler);
}

void scheduler_shutdown(void){
	// join scheduler thread
	mtx.lock();
	running = false;
	cond.notify_one();
	mtx.unlock();
	thr.join();

	// release entries
	entries.clear();
}

SchRef scheduler_add(int64 delay, Dispatcher *disp, const Task &task){
	return scheduler_add(delay, disp, Task(task));
}

SchRef scheduler_add(int64 delay, Dispatcher *disp, Task &&task){
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	auto ret = entries.emplace(next_id++, time, disp, std::move(task));
	if(!ret.second){
		LOG("scheduler_add: failed to insert new entry");
		return SCHREF_INVALID;
	}
	if(ret.first == entries.begin())
		cond.notify_one();
	return SchRef(ret.first->id, ret.first->time);
}

bool scheduler_remove(const SchRef &ref){
	std::lock_guard<std::mutex> lock(mtx);
	auto entry = entries.find(ref);
	if(entry == entries.end()){
		LOG("scheduler_remove: trying to remove invalid entry");
		return false;
	}
	entries.erase(entry);
	return true;
}

bool scheduler_reschedule(int64 delay, SchRef &ref){
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	auto old = entries.find(ref);
	if(old == entries.end()){
		LOG("scheduler_reschedule: trying to reschedule invalid entry");
		return false;
	}
	auto ret = entries.emplace(next_id++, time, old->disp, std::move(old->task));
	if(!ret.second){
		LOG("scheduler_reschedule: failed to re-insert entry");
		return false;
	}
	// remove old entry and update SchRef
	entries.erase(old);
	ref.id = ret.first->id;
	ref.time = ret.first->time;
	if(ret.first == entries.begin())
		cond.notify_one();
	return true;
}
