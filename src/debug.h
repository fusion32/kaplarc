#ifndef DEBUG_H_
#define DEBUG_H_

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

#endif //DEBUG_H_

