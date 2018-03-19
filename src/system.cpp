#include <chrono>
#include <thread>
#include "platform.h"
#include "system.h"

int64 sys_tick_count(void){
	auto now = std::chrono::steady_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch());
	return dur.count();
}

int sys_cpu_count(void){
	return std::thread::hardware_concurrency();
}
