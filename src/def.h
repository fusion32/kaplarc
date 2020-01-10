#ifndef DEF_H_
#define DEF_H_


#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

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

#define ARRAY_SIZE(a)		(sizeof(a)/sizeof((a)[0]))
#define OFFSET_POINTER(ptr, x)	((void*)(((char*)(ptr)) + (x)))

#define IS_POWER_OF_TWO(x)	((x != 0) && ((x & (x - 1)) == 0))

#define MIN(x, y)		((x < y) ? (x) : (y))
#define MAX(x, y)		((x > y) ? (x) : (y))

// arch settings (these options should be ajusted to the arch being used)
//#define ARCH_BIG_ENDIAN 1
#define ARCH_UNALIGNED_ACCESS 1

// platform settings
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_FREEBSD)	\
		&& !defined(PLATFORM_WINDOWS)
#	if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
#		define PLATFORM_WINDOWS 1
#	elif defined(__FreeBSD__)
#		define PLATFORM_FREEBSD 1
#	elif defined(__linux__)
#		define PLATFORM_LINUX 1
#	else
#		error platform not supported
#	endif
#endif

// compiler settings
#if defined(_MSC_VER)
#	define INLINE __forceinline
#elif defined(__GNUC__)
#	define INLINE __attribute__((always_inline))
#else
#	define INLINE inline
#endif

// debug settings
#ifdef _DEBUG
#	define BUILD_DEBUG 1
#endif
#ifndef BUILD_DEBUG
#	define DEBUG_LOG(...)		((void)0)
#	define DEBUG_CHECK(...)		((void)0)
#	define ASSERT(...)		((void)0)
#	define UNREACHABLE()		((void)0)
#else
#	include "log.h"
#	define DEBUG_LOG(...)		log_add("DEBUG", __VA_ARGS__)
#	define DEBUG_CHECK(cond, ...)	if(!(cond)){ DEBUG_LOG(__VA_ARGS__); }

#	ifdef NDEBUG
#		undef NDEBUG
#	endif
#	include <assert.h>
#	define ASSERT(expr)		assert(expr)
#	define UNREACHABLE()		ASSERT(0 && "unreachable")
#endif

// database settings
#ifndef DB_PGSQL
#	define DB_PGSQL 1
#endif

#endif //DEF_H_
