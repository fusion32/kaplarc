#include "dispatcher.h"
#include "log.h"
#include "ringbuffer.h"
#include "system.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

struct Dispatcher{
	RingBuffer<Task, 0x8000> rb;
	std::thread thr;
	std::mutex mtx;
	std::condition_variable cond;
	bool running;

	Dispatcher(void){}
	~Dispatcher(void){
		// release std::function resources
		while(!rb.empty()){
			rb.front() = nullptr;
			rb.pop();
		}
	}
};

static void dispatcher_thread(Dispatcher *d){
	Task task;
	std::unique_lock<std::mutex> lock(d->mtx, std::defer_lock);
	while(d->running){
		lock.lock();
		if(d->rb.empty()){
			d->cond.wait(lock);
			if(d->rb.empty()){
				lock.unlock();
				continue;
			}
		}

		task = std::move(d->rb.front());
		d->rb.pop();
		lock.unlock();

		// execute task
		task();
		// release std::function resources
		task = nullptr;
	}
}

Dispatcher *dispatcher_create(void){
	Dispatcher *d = new Dispatcher;
	d->running = true;
	d->thr = std::thread(dispatcher_thread, d);
	return d;
}

void dispatcher_destroy(Dispatcher *d){
	d->mtx.lock();
	d->running = false;
	d->cond.notify_one();
	d->mtx.unlock();
	d->thr.join();
	delete d;
}

void dispatcher_add(Dispatcher *d, const Task &task){
	dispatcher_add(d, Task(task));
}

void dispatcher_add(Dispatcher *d, Task &&task){
	std::lock_guard<std::mutex> lock(d->mtx);
	if(!d->rb.push(std::move(task)))
		LOG_ERROR("dispatcher_add: task ring buffer is at maximum capacity (%d)", d->rb.capacity());
	else
		d->cond.notify_one();
}
