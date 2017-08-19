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
	SchEntry	*entry;

	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		if(tree.empty()){
			cond.wait(ulock);
			if(tree.empty()){
				ulock.unlock();
				continue;
			}
		}

		entry = tree.min();
		delta = entry->time - sys_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			entry = tree.min();
			if(entry == nullptr || entry->time > sys_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove entry
		wrk = std::move(entry->wrk);
		tree.remove(*entry);
		ulock.unlock();

		// dispatch work
		work_dispatch(wrk);
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

SchRef scheduler_add(int64 delay, Work wrk){
	int64 time = delay + sys_tick_count();
	SchEntry *entry;

	std::lock_guard<std::mutex> lguard(mtx);
	entry = tree.emplace(next_id++, time, std::move(wrk));
	if(entry == nullptr){
		LOG("scheduler_add: scheduler is at maximum capacity (%d)", SCH_MAX_ENTRIES);
		return SCHREF_INVALID;
	}

	if(entry == tree.min())
		cond.notify_one();
	return SchRef(entry->id, entry->time);
}

bool scheduler_remove(const SchRef &ref){
	std::lock_guard<std::mutex> lguard(mtx);
	if(!tree.remove(ref)){
		LOG("scheduler_remove: trying to remove invalid scheduler entry");
		return false;
	}
	return true;
}