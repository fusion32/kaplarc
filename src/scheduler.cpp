#include <chrono>
#include <condition_variable>
#include <thread>

#include "scheduler.h"
#include "avltree.h"
#include "log.h"
#include "system.h"
#include "work.h"

struct SchEntry{
	int64 id;
	int64 time;
	Work wrk;

	// constructors
	SchEntry(void){}

	template<typename G>
	SchEntry(int64 id_, int64 time_, G &&wrk_)
	  : id(id_), time(time_), wrk(std::forward<G>(wrk_)) {}

	// comparisson with `SchEntry`
	bool operator==(const SchEntry &rhs) const{
		return id == rhs.id;
	}
	bool operator!=(const SchEntry &rhs) const{
		return id != rhs.id;
	}
	bool operator<(const SchEntry &rhs) const{
		return time < rhs.time;
	}
	bool operator>(const SchEntry &rhs) const{
		return time > rhs.time;
	}

	// comparisson with `SchRef`
	bool operator==(const SchRef &rhs) const{
		return id == rhs.id;
	}
	bool operator!=(const SchRef &rhs) const{
		return id != rhs.id;
	}
	bool operator<(const SchRef &rhs) const{
		return time < rhs.time;
	}
	bool operator>(const SchRef &rhs) const{
		return time > rhs.time;
	}
};

// scheduler entries
#define SCH_MAX_ENTRIES 0x3FFF
static AVLTree<SchEntry, SCH_MAX_ENTRIES> tree;

// scheduler control
static std::thread		thr;
static std::mutex		mtx;
static std::condition_variable	cond;
static int64			next_id = 1;
static bool			running = false;

static void scheduler(void){
	int64		delta;
	Work		wrk;
	auto		entry = tree.end();

	std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
	while(running){
		lock.lock();
		if(tree.empty()){
			cond.wait(lock);
			if(tree.empty()){
				lock.unlock();
				continue;
			}
		}

		entry = tree.begin();
		delta = entry->time - sys_tick_count();
		if(delta > 0){
			cond.wait_for(lock, std::chrono::milliseconds(delta));
			entry = tree.begin();
			if(entry == tree.end() || entry->time > sys_tick_count()){
				lock.unlock();
				continue;
			}
		}

		// get work and remove entry
		wrk = std::move(entry->wrk);
		tree.remove(entry);
		lock.unlock();

		work_dispatch(std::move(wrk));
	}
}

void scheduler_init(void){
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
}

SchRef scheduler_add(int64 delay, const Work &wrk){
	return scheduler_add(delay, Work(wrk));
}

SchRef scheduler_add(int64 delay, Work &&wrk){
	int64 time = delay + sys_tick_count();

	std::lock_guard<std::mutex> lock(mtx);
	auto entry = tree.emplace(next_id++, time, std::move(wrk));
	if(entry == tree.end()){
		LOG("scheduler_add: scheduler is at maximum capacity (%d)", SCH_MAX_ENTRIES);
		return SCHREF_INVALID;
	}

	if(entry == tree.begin())
		cond.notify_one();
	return SchRef(entry->id, entry->time);
}

bool scheduler_remove(const SchRef &ref){
	std::lock_guard<std::mutex> lock(mtx);
	auto it = tree.find(ref);
	if(it == tree.end()){
		LOG("scheduler_remove: trying to remove invalid entry");
		return false;
	}
	tree.remove(it);
	return true;
}

bool scheduler_reschedule(int64 delay, SchRef &ref){
	int64 time = delay + sys_tick_count();
	std::lock_guard<std::mutex> lock(mtx);
	auto old = tree.find(ref);
	if(old == tree.end()){
		LOG("shceduler_reschedule: trying to reschedule invalid entry");
		return false;
	}

	auto entry = tree.emplace(next_id++, time, std::move(old->wrk));
	if(entry == tree.end()){
		LOG("scheduler_reschedule: failed to re-insert entry"
			"(scheduler is at maximum capactity `%d`)", SCH_MAX_ENTRIES);
		return false;
	}

	// remove old entry and update SchRef
	tree.remove(old);
	ref.id = entry->id;
	ref.time = time;

	if(entry == tree.begin())
		cond.notify_one();
	return true;
}
