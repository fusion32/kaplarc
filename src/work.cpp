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

static void worker(void){
	Work wrk;
	std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
	while(running){
		lock.lock();
		if(rb.size() == 0){
			cond.wait(lock);
			if(rb.size() == 0){
				lock.unlock();
				continue;
			}
		}

		wrk = std::move(rb.front());
		rb.pop();
		lock.unlock();

		// execute work
		wrk();
		// release std::function resources
		wrk = nullptr;
	}
}

void work_init(void){
	int count = sys_cpu_count() - 1;
	if(count <= 0)
		count = 1;
	running = true;
	for(int i = 0; i < count; i++)
		thread_pool.emplace_back(worker);
}

void work_shutdown(void){
	mtx.lock();
	running = false;
	cond.notify_all();
	mtx.unlock();

	for(std::thread &thr : thread_pool)
		thr.join();
	thread_pool.clear();
}

void work_dispatch(Work wrk){
	std::lock_guard<std::mutex> lock(mtx);
	if(!rb.push(std::move(wrk)))
		LOG_ERROR("work_dispatch: work ring buffer is at maximum capacity (%d)", MAX_WORK);
	else
		cond.notify_one();
}

void work_multi_dispatch(int count, const Work &wrk){
	std::lock_guard<std::mutex> lock(mtx);
	if(rb.size() + count >= MAX_WORK){
		LOG_ERROR("work_dispatch_array: requested amount of work would overflow the ringbuffer");
		return;
	}

	// push work into the ringbuffer
	for(int i = 0; i < count; ++i)
		rb.push(wrk);

	// signal worker threads
	cond.notify_all();
}

void work_multi_dispatch(int count, const Work *wrk){
	std::lock_guard<std::mutex> lock(mtx);
	if(rb.size() + count >= MAX_WORK){
		LOG_ERROR("work_dispatch_array: requested amount of work would overflow the ringbuffer");
		return;
	}

	// push work into the ringbuffer
	for(int i = 0; i < count; i++)
		rb.push(wrk[i]);

	// signal worker threads
	cond.notify_all();
}
