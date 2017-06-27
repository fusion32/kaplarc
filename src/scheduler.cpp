#include "scheduler.h"

#include "avl_tree.h"
#include "log.h"
#include "system.h"
#include "work.h"

#include <chrono>
#include <condition_variable>
#include <thread>

namespace kp{
struct schentry{
	int64 id;
	int64 time;
	kp::work wrk;

	// constructors
	schentry(void){}

	template<typename G>
	schentry(int64 id_, int64 time_, G &&wrk_)
		: id(id_), time(time_), wrk(std::forward<G>(wrk_)) {}

	// comparisson with `schentry`
	bool operator==(const schentry &rhs) const{
		return id == rhs.id;
	}
	bool operator!=(const schentry &rhs) const{
		return id != rhs.id;
	}
	bool operator<(const schentry &rhs) const{
		return time < rhs.time;
	}
	bool operator>(const schentry &rhs) const{
		return time > rhs.time;
	}

	// comparisson with `schref`
	bool operator==(const schref &rhs) const{
		return id == rhs.id;
	}
	bool operator!=(const schref &rhs) const{
		return id != rhs.id;
	}
	bool operator<(const schref &rhs) const{
		return time < rhs.time;
	}
	bool operator>(const schref &rhs) const{
		return time > rhs.time;
	}
};
} //namespace

// scheduler entries
#define SCH_MAX_ENTRIES 4096
static kp::avl_tree<kp::schentry, SCH_MAX_ENTRIES> tree;

// scheduler control
static std::thread		thr;
static std::mutex		mtx;
static std::condition_variable	cond;
static int64			next_id = 1;
static bool			running = false;

static void scheduler(void)
{
	int64 delta;
	kp::work wrk;
	kp::schentry *entry;

	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		entry = tree.min();
		if(entry == nullptr){
			cond.wait(ulock);
			entry = tree.min();
			if(entry == nullptr){
				ulock.unlock();
				continue;
			}
		}

		delta = entry->time - sys_get_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			entry = tree.min();
			if(entry == nullptr || entry->time > sys_get_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove entry
		LOG("entry: %d", entry->id);
		wrk = std::move(entry->wrk);
		tree.remove(*entry);
		ulock.unlock();

		// dispatch work
		work_dispatch(wrk);
	}
}


void scheduler_init(void)
{
	// spawn scheduler thread
	running = true;
	thr = std::thread(scheduler);
}

void scheduler_shutdown(void)
{
	// join scheduler thread
	mtx.lock();
	running = false;
	cond.notify_one();
	mtx.unlock();
	thr.join();
}

kp::schref scheduler_add(int64 delay, kp::work work_)
{
	int64 time = delay + sys_get_tick_count();
	kp::schentry *entry;

	std::lock_guard<std::mutex> lguard(mtx);
	entry = tree.emplace(next_id++, time, std::move(work_));
	if(entry == tree.min())
		cond.notify_one();
	return {entry->id, entry->time};
}

bool scheduler_remove(const kp::schref &ref)
{
	std::lock_guard<std::mutex> lguard(mtx);
	if(!tree.remove(ref)){
		LOG("scheduler_remove: trying to remove invalid scheduler entry");
		return false;
	}
	return true;
}