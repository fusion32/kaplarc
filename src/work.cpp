#include <vector>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "work.h"
#include "log.h"
#include "ringbuffer.h"
#include "system.h"

#define MAX_WORK 0xFFFF
static RingBuffer<Work, MAX_WORK>	rb;
static std::vector<std::thread>		thread_pool;
static std::mutex			mtx;
static std::condition_variable		cond;
static bool				running = false;

static void worker(void)
{
	Work wrk;
	std::unique_lock<std::mutex> ulock(mtx, std::defer_lock);
	while(running){
		ulock.lock();
		if(rb.size() == 0){
			cond.wait(ulock);
			if(rb.size() == 0){
				ulock.unlock();
				continue;
			}
		}

		wrk = std::move(*rb.pop());
		ulock.unlock();

		wrk();
	}
}

void work_init(void)
{
	int count = sys_get_cpu_count() - 1;
	if(count <= 0)
		count = 1;
	running = true;
	for(int i = 0; i < count; i++)
		thread_pool.emplace_back(worker);
}

void work_shutdown(void)
{
	mtx.lock();
	running = false;
	cond.notify_all();
	mtx.unlock();

	for(std::thread &thr : thread_pool)
		thr.join();
	thread_pool.clear();
}

void work_dispatch(Work wrk)
{
	std::lock_guard<std::mutex> lguard(mtx);
	if(!rb.push(std::move(wrk)))
		LOG_ERROR("work_dispatch: work ring buffer is at maximum capacity (%d)", MAX_WORK);
	else
		cond.notify_one();
}

void work_dispatch_array(int count, bool single, Work *wrk)
{
	std::lock_guard<std::mutex> lguard(mtx);
	if(rb.size() + count >= MAX_WORK){
		LOG_ERROR("work_dispatch_array: requested amount of work would cause the ring buffer to overflow");
		return;
	}

	for(int i = 0; i < count; i++){
		// NOTE: don't move here or else the function
		// handles will be all empty after the dispatch
		rb.push(*wrk);
		// if there is a single work in the array
		// keep adding it to the work pool, else
		// advance to the next element
		if(!single)
			wrk++;
	}

	// signal worker threads
	cond.notify_all();
}
