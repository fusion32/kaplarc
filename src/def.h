#ifndef DEF_H_
#define DEF_H_

#include <stdint.h>
typedef unsigned int	uint;
typedef int8_t		int8;
typedef uint8_t		uint8;
typedef int16_t		int16;
typedef uint16_t	uint16;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;
typedef uintptr_t	uintptr;

template<typename T, size_t N>
constexpr size_t array_size(T (&arr)[N]){
	return N;
}

constexpr bool is_power_of_two(size_t size){
	return (size != 0) && ((size & (size - 1)) == 0);
}

// arch settings (these options should be ajusted to the arch being used)
//#define ARCH_BIG_ENDIAN 1
#define ARCH_UNALIGNED_ACCESS 1

// platform settings
#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WINDOWS 1
#elif defined(__FreeBSD__)
#	define PLATFORM_BSD 1
#	define PLATFORM_FREEBSD 1
#elif defined(__linux__)
#	define PLATFORM_LINUX 1
#else
#	error platform not supported
#endif

// debug settings
#ifndef _DEBUG
#	define DEBUG_LOG(...) ((void)0)
#	define DEBUG_CHECK(...) ((void)0)
#	define UNREACHABLE() ((void)0)
#else
#	include "log.h"
#	define DEBUG_LOG(...) log_add("DEBUG", __VA_ARGS__)
#	define DEBUG_CHECK(cond, ...) if(!(cond)) DEBUG_LOG(__VA_ARGS__);

#	ifdef NDEBUG
#		undef NDEBUG
#	endif
#	include <assert.h>
#	define UNREACHABLE() assert(0 && "unreachable")
#endif

// database settings
#if defined(__DB_CASSANDRA__)
#	if defined(__DB_PGSQL__)
#		undef __DB_PGSQL__
#	endif
#elif defined(__DB_PGSQL__)
#	error PGSQL DB Interface not yet implemented.
#else
#	define __DB_CASSANDRA__ 1
#endif

#endif //DEF_H_
