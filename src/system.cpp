#include "system.h"
#include <chrono>
#include <thread>

int64 sys_get_tick_count()
{
	auto now = std::chrono::steady_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch());
	return dur.count();
}

int32 sys_get_cpu_count()
{
	return std::thread::hardware_concurrency();
}
