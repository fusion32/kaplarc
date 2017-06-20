#include "scheduler.h"

#include "log.h"
#include "mmblock.h"
#include "system.h"
#include "work.h"

#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <list>
#include <thread>

namespace kp{
struct sch_entry{
	int64 time;
	kp::work wrk;
	sch_entry *next;
};
} //namespace

#define SCH_MAX_LIST_SIZE 4096
static kp::sch_entry		*head;
static kp::mmblock<kp::sch_entry, SCH_MAX_LIST_SIZE> blk;

static std::thread		thr;
static std::mutex		mtx;
static std::condition_variable	cond;
static bool			running = false;

static void scheduler(void)
{
	int64 delta;
	kp::work wrk;
	kp::sch_entry *tmp;

	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		if(head == nullptr){
			cond.wait(ulock);
			if(head == nullptr){
				ulock.unlock();
				continue;
			}
		}

		delta = head->time - sys_get_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			if(head == nullptr || head->time > sys_get_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// get work and remove head
		wrk = std::move(head->wrk);
		tmp = head;
		head = head->next;
		blk.free(tmp);
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

kp::sch_entry *scheduler_add(long delay, kp::work work_)
{
	kp::sch_entry *entry, **it;
	int64 time = delay + sys_get_tick_count();

	std::lock_guard<std::mutex> lguard(mtx);
	// allocate entry
	entry = blk.alloc();
	if(entry == nullptr){
		LOG_ERROR("scheduler_add: list is at maximum capacity (%d)", SCH_MAX_LIST_SIZE);
		return nullptr;
	}
	// insert entry, ordered by time
	it = &head;
	while(*it != nullptr && (*it)->time < time)
		it = &(*it)->next;
	entry->time = time;
	entry->wrk = std::move(work_);
	entry->next = *it;
	*it = entry;
	// signal scheduler if head has changed
	if(head == entry)
		cond.notify_one();
	return entry;
}

bool scheduler_remove(kp::sch_entry *entry)
{
	kp::sch_entry **it;

	std::lock_guard<std::mutex> lguard(mtx);
	// check if entry is still valid
	it = &head;
	while(*it != nullptr && (*it) != entry)
		it = &(*it)->next;
	if(*it == nullptr){
		LOG_WARNING("scheduler_remove: trying to remove invalid entry");
		return false;
	}
	// remove entry
	*it = (*it)->next;
	blk.free(entry);
	return true;
}

bool scheduler_reschedule(long delay, kp::sch_entry *entry)
{
	kp::sch_entry **it;
	int64 time = delay + sys_get_tick_count();

	std::lock_guard<std::mutex> lguard(mtx);
	// check if entry is still valid
	it = &head;
	while(*it != nullptr && *it != entry)
		it = &(*it)->next;
	if(*it == nullptr){
		LOG_WARNING("scheduler_reschedule: trying to reschedule invalid entry");
		return false;
	}
	// re-insert entry with new time
	*it = (*it)->next;
	if(entry->time > time)
		it = &head;
	while(*it != nullptr && (*it)->time < time)
		it = &(*it)->next;
	entry->time = time;
	entry->next = *it;
	*it = entry;
	// signal scheduler if head has changed
	if(head == entry)
		cond.notify_one();
	return true;
}

bool scheduler_pop(kp::sch_entry *entry)
{
	kp::sch_entry **it;

	std::lock_guard<std::mutex> lguard(mtx);
	// check if entry is still valid
	it = &head;
	while(*it != nullptr && *it != entry)
		it = &(*it)->next;
	if(*it == nullptr){
		LOG_WARNING("scheduler_pop: trying to pop invalid entry");
		return false;
	}
	// re-insert entry at the start
	*it = (*it)->next;
	entry->time = 0;
	entry->next = head;
	head = entry;
	// signal scheduler
	cond.notify_one();
	return true;
}
