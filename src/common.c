#include "common.h"
#include "log.h"

#ifdef PLATFORM_WINDOWS
#	define WIN32_LEAN_AND_MEAN 1
#	include <windows.h>
#else
#	include <time.h>
#	include <unistd.h>
#endif

int64 kpl_clock_monotonic_msec(void){
#ifdef PLATFORM_WINDOWS
	return (int64)GetTickCount64();
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64)ts.tv_sec * 1000 +
		(int64)ts.tv_nsec / 1000000;
#endif
}

void kpl_sleep_msec(int64 ms){
#ifdef PLATFORM_WINDOWS
	DEBUG_ASSERT(ms < MAXDWORD && "windows limitation");
	Sleep((DWORD)ms);
#else
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#endif
}

int kpl_cpu_count(void){
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

void kpl_abort(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	log_addv("SYS_ABORT", fmt, ap);
	va_end(ap);
	abort();
}

void *kpl_malloc(size_t size){
	void *ptr = malloc(size);
	if(ptr == NULL)
		kpl_abort("kpl_malloc: out of memory");
	return ptr;
}

void *kpl_realloc(void *ptr, size_t size){
	void *newptr = realloc(ptr, size);
	if(newptr == NULL)
		kpl_abort("kpl_realloc: out of memory");
	return newptr;
}

void kpl_free(void *ptr){
	free(ptr);
}

// strncpy
//	- <dst> is a char buffer of size dstlen
//	- src must be a valid cstring
void kpl_strncpy(char *dst, size_t dstsize, const char *src){
	size_t cpylen = strlen(src);
	if(cpylen >= dstsize)
		cpylen = dstsize - 1;
	memcpy(dst, src, cpylen);
	dst[cpylen] = 0x00;
}

// strncat
//	- <dst> must be a cstring with maximum capacity of <dstsize>
//	- <src> must be a cstring
void kpl_strncat(char *dst, size_t dstsize, const char *src){
	// if dst is a valid nul-terminated string,
	// dstsize - dstlen cannot underflow
	size_t dstlen = strlen(dst);
	kpl_strncpy(dst + dstlen, dstsize - dstlen, src);
}

// strncat_n
//	- <dst> must be a cstring with maximum capacity of <dstsize>
//	- <...> is a NULL-terminated list of cstrings
void kpl_strncat_n(char *dst, size_t dstsize, ...){
	const char *src;
	va_list ap;

	va_start(ap, dstsize);
	while(1){
		src = va_arg(ap, const char*);
		if(src == NULL)
			break;
		kpl_strncat(dst, dstsize, src);
	}
	va_end(ap);
}
