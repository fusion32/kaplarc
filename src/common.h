#ifndef KAPLAR_COMMON_H_
#define KAPLAR_COMMON_H_ 1

// stdlib base
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// always enable assert
#ifdef NDEBUG
#	undef NDEBUG
#endif
#include <assert.h>
#define ASSERT(expr) assert(expr)
#define UNREACHABLE() ASSERT(0 && "unreachable")

// int types
typedef int8_t		int8;
typedef uint8_t		uint8;
typedef int16_t		int16;
typedef uint16_t	uint16;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int64_t		int64;
typedef uint64_t	uint64;
typedef uintptr_t	uintptr;
typedef ptrdiff_t	ptrdiff;

// @REMOVE
#define PLATFORM_WINDOWS 1
#define ARCH_BIG_ENDIAN 0
#define ARCH_UNALIGNED_ACCESS 1
#define ARCH_CACHE_LINE_SIZE 64

// arch settings (these options should be ajusted to the arch being used)
#ifndef ARCH_BIG_ENDIAN
#	define ARCH_BIG_ENDIAN 0
#endif
#ifndef ARCH_UNALIGNED_ACCESS
#	define ARCH_UNALIGNED_ACCESS 0
#endif
#ifndef ARCH_CACHE_LINE_SIZE
#	define ARCH_CACHE_LINE_SIZE 64
#endif

// platform settings
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_FREEBSD)	\
		&& !defined(PLATFORM_WINDOWS)
#error "platform must be defined"
#endif

// compiler settings
#if defined(_MSC_VER)
#	include <intrin.h>
#	define INLINE __forceinline
#	define _CLZ32(x) ((int)__lzcnt(x))
#	define _CLZ64(x) ((int)__lzcnt64(x))
#	define _POPCNT32(x) ((int)__popcnt(x))
#	define _POPCNT64(x) ((int)__popcnt64(x))
#	ifdef _WIN64
#		define COMPILER_ENV64 1
#	else
#		define COMPILER_ENV32 1
#	endif
#elif defined(__GNUC__)
#	define INLINE __attribute__((always_inline))
#	define _CLZ32(x) ((int)__builtin_clzl(x))
#	define _CLZ64(x) ((int)__builtin_clzll(x))
#	define _POPCNT32(x) ((int)__builtin_popcountl(x))
#	define _POPCNT64(x) ((int)__builtin_popcountll(x))
#	ifdef __x86_64__
#		define COMPILER_ENV64 1
#	else
#		define COMPILER_ENV32 1
#	endif
#else
#	error "compiler not supported"
#endif

// debug settings
#ifdef _DEBUG
#	define BUILD_DEBUG 1
#endif
#ifndef BUILD_DEBUG
#	define DEBUG_LOG(...)		((void)0)
#	define DEBUG_CHECK(...)		((void)0)
#	define DEBUG_ASSERT(...)	((void)0)
#else
#	include "log.h"
#	define DEBUG_LOG(...)		log_add("DEBUG", __VA_ARGS__)
#	define DEBUG_CHECK(cond, ...)	if(!(cond)){ DEBUG_LOG(__VA_ARGS__); }
#	define DEBUG_ASSERT(expr)	ASSERT(expr)
#endif

// common macros
#define ARRAY_SIZE(a)		(sizeof(a)/sizeof((a)[0]))
#define OFFSET_POINTER(ptr, x)	((void*)(((char*)(ptr)) + (x)))
#define IS_POWER_OF_TWO(x)	((x != 0) && ((x & (x - 1)) == 0))
#define MIN(x, y)		((x < y) ? (x) : (y))
#define MAX(x, y)		((x > y) ? (x) : (y))

// adler32.c
// -----------------------------------------------
uint32 adler32(const uint8 *buf, size_t len);

// murmur2.c
// -----------------------------------------------
uint32 murmur2_32(const uint8 *data, size_t len, uint32 seed);

// mem_arena.c
// -----------------------------------------------
#define MEM_PTR_ALIGNMENT sizeof(void*)
#define MEM_PTR_ALIGNMENT_MASK (sizeof(void*) - 1)
struct mem_arena{
	size_t block_size;
	struct mem_arena_block *head;
};
#define MEM_ARENA_INIT(s) {.block_size = (s), .head = NULL}
void mem_arena_cleanup(struct mem_arena *arena);
void *mem_arena_alloc(struct mem_arena *arena, size_t size);
void mem_arena_rollback(struct mem_arena *arena);
void mem_arena_reset(struct mem_arena *arena);

// common.c
// -----------------------------------------------
// system wrappers
int64 kpl_clock_monotonic_msec(void);
void kpl_sleep_msec(int64 ms);
int kpl_cpu_count(void);
void kpl_abort(const char *fmt, ...);
// stdlib wrappers and replacements
void *kpl_malloc(size_t size);
void *kpl_realloc(void *ptr, size_t size);
void kpl_free(void *ptr);
void kpl_strncpy(char *dst, size_t dstsize, const char *src);
void kpl_strncat(char *dst, size_t dstsize, const char *src);
void kpl_strncat_n(char *dst, size_t dstsize, ...);


#endif //KAPLAR_SYSTEM_H_
