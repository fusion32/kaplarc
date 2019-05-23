#include "dispatcher.h"
#include "log.h"
#include "ringbuffer.h"
#include "system.h"

#include <condition_variable>
#include <mutex>
#include <thread>

static RingBuffer<Task, 0x8000> rb;
static std::thread thr;
static std::mutex mtx;
static std::condition_variable cond;
static bool running;

static void dispatcher_thread(void){
	Task task;
	std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
	while(running){
		lock.lock();
		if(rb.empty()){
			cond.wait(lock);
			if(rb.empty()){
				lock.unlock();
				continue;
			}
		}
		task = std::move(rb.front());
		rb.pop();
		lock.unlock();

		// execute task
		task();
		// release task
		task = nullptr;
	}
}

void dispatcher_init(void){
	running = true;
	thr = std::thread(dispatcher_thread);
}

void dispatcher_shutdown(void){
	// join dispatcher thread
	mtx.lock();
	running = false;
	cond.notify_one();
	mtx.unlock();
	thr.join();

	// release tasks
	while(!rb.empty()){
		rb.front() = nullptr;
		rb.pop();
	}
}

void dispatcher_add(const Task &task){
	dispatcher_add(Task(task));
}

void dispatcher_add(Task &&task){
	std::lock_guard<std::mutex> lock(mtx);
	if(!rb.push(std::move(task)))
		LOG_ERROR("dispatcher_add: task ring buffer is"
			" at maximum capacity (%d)", rb.capacity());
	else
		cond.notify_one();
}
