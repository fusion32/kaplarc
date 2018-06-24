#include "def.h"
#include <chrono>
#include <thread>
#include <stdlib.h>

int64 sys_tick_count(void){
	auto now = std::chrono::steady_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch());
	return dur.count();
}

int sys_cpu_count(void){
	return std::thread::hardware_concurrency();
}

void *sys_aligned_alloc(size_t alignment, size_t size){
	void *ptr;

	DEBUG_CHECK(is_power_of_two(alignment),
		"sys_aligned_alloc: `alignment` must be a power of two");

#ifdef PLATFORM_WINDOWS
	ptr = _aligned_malloc(size, alignment);
#else
	if(posix_memalign(&ptr, alignment, size) != 0)
		ptr = nullptr;
#endif
	return ptr;
}

void sys_aligned_free(void *ptr){
#ifdef PLATFORM_WINDOWS
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}
