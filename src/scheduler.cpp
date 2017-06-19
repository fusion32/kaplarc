#include "scheduler.h"

#include "log.h"
#include "system.h"
#include "work.h"

#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <thread>

static std::forward_list<kp::sch_work>	list;
static std::thread			thr;
static std::mutex			mtx;
static std::condition_variable		cond;
static bool				running = false;

static void scheduler(void)
{
	long delta;
	kp::sch_work work;
	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);

	while(running){
		ulock.lock();
		if(list.empty()){
			cond.wait(ulock);
			if(list.empty()){
				ulock.unlock();
				continue;
			}
		}

		delta = list.front().time - sys_get_tick_count();
		if(delta > 0){
			cond.wait_for(ulock, std::chrono::milliseconds(delta));
			if(list.empty() || list.front().time > sys_get_tick_count()){
				ulock.unlock();
				continue;
			}
		}

		// copy and pop head
		work = list.front();
		list.pop_front();
		ulock.unlock();

		// dispatch to worker threads
		work_dispatch(work.wrk);
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

kp::sch_entry scheduler_add(long delay, kp::work work_)
{
	long time = delay + sys_get_tick_count();
	kp::sch_work work{time, std::move(work_)};

	std::lock_guard<std::mutex> lguard(mtx);
	if(list.empty())
		cond.notify_one();

	auto cur = list.cbefore_begin();
	auto prev = cur++;
	while(cur != list.end() && cur->time < time){
		++prev; ++cur;
	}
	return list.insert_after(prev, std::move(work));
}

bool scheduler_remove(const kp::sch_entry &entry)
{
	std::lock_guard<std::mutex> lguard(mtx);
	auto cur = list.cbefore_begin();
	auto prev = cur++;
	while(cur != list.end() && cur != entry){
		++prev; ++cur;
	}

	if(cur == list.end()){
		LOG_WARNING("scheduler_remove: trying to remove invalid entry");
		return false;
	}

	list.erase_after(prev);
	return true;
}

bool scheduler_reschedule(long delay, kp::sch_entry &entry)
{
	long time = delay + sys_get_tick_count();
	kp::sch_work work;

	std::lock_guard<std::mutex> lguard(mtx);
	// retrieve entry from the list
	auto cur = list.cbefore_begin();
	auto prev = cur++;
	while(cur != list.end() && cur != entry){
		++prev; ++cur;
	}
	if(cur == list.end()){
		LOG_WARNING("scheduler_reschedule: trying to reschedule invalid entry");
		return false;
	}

	// remove entry
	work = std::move(*cur);
	list.erase_after(prev);

	// reset iteration if new time is lower
	if(work.time > time){
		cur = list.cbefore_begin();
		prev = cur++;
	}

	// re-insert work
	while(cur != list.end() && cur->time < time){
		++prev; ++cur;
	}
	work.time = time;
	entry = list.insert_after(prev, std::move(work));
	if(entry == list.begin())
		cond.notify_one();
	return true;
}

bool scheduler_pop(kp::sch_entry &entry)
{
	kp::sch_work work;

	std::lock_guard<std::mutex> lguard(mtx);
	auto cur = list.cbefore_begin();
	auto prev = cur++;
	while(cur != list.end() && cur != entry){
		++prev; ++cur;
	}
	if(cur == list.end()){
		LOG_WARNING("scheduler_pop: trying to pop invalid entry");
		return false;
	}

	work = std::move(*cur);
	list.erase_after(prev);

	work.time = 0;
	list.push_front(std::move(work));
	entry = list.cbegin();
	cond.notify_one();
	return false;
}
