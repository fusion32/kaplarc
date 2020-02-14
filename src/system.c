#include "system.h"
#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#ifdef PLATFORM_WINDOWS
#	include <windows.h>
#else
#	include <time.h>
#	include <unistd.h>
#endif

int64 sys_tick_count(void){
#ifdef PLATFORM_WINDOWS
	return GetTickCount64();
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

void sys_abort(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	log_add1("SYS_ABORT", fmt, ap);
	va_end(ap);
	abort();
}

void *sys_malloc(size_t size){
	void *ptr = malloc(size);
	if(ptr == NULL)
		sys_abort("sys_malloc: out of memory");
	return ptr;
}

void *sys_realloc(void *ptr, size_t size){
	return realloc(ptr, size);
}

void sys_free(void *ptr){
	free(ptr);
}

void *sys_aligned_malloc(size_t size, size_t alignment){
	DEBUG_ASSERT(IS_POWER_OF_TWO(alignment) &&
		"sys_aligned_alloc: `alignment` must be a power of two");

	void *ptr;
#ifdef PLATFORM_WINDOWS
	ptr = _aligned_malloc(size, alignment);
	if(ptr == NULL)
		sys_abort("sys_aligned_malloc: out of memory");
	return ptr;
#else
	if(posix_memalign(&ptr, alignment, size) != 0)
		sys_abort("sys_aligned_malloc: out of memory");
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
