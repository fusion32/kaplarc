#include "system.h"

#ifdef PLATFORM_WINDOWS
#	include <windows.h>
#else
#	include <time.h>
#	include <unistd.h>
#endif

int64 sys_tick_count(void){
#ifdef PLATFORM_WINDOWS
	return GetTickCount();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000) +
		(ts.tv_usec / 1000000);
#endif
}

int sys_cpu_count(void){
#ifdef PLATFORM_WINDOWS
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#else
	// this option is available on FreeBSD
	// since version 5.0
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

void *sys_aligned_alloc(size_t alignment, size_t size){
	ASSERT(IS_POWER_OF_TWO(alignment) &&
		"sys_aligned_alloc: `alignment` must be a power of two");

#ifdef PLATFORM_WINDOWS
	return _aligned_malloc(size, alignment);
#else
	void *ptr;
	if(posix_memalign(&ptr, alignment, size) != 0)
		return NULL;
	return ptr;
#endif
}

void sys_aligned_free(void *ptr){
#ifdef PLATFORM_WINDOWS
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}
