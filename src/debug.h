#ifndef DEBUG_H_
#define DEBUG_H_

#ifndef _DEBUG
#	define DEBUG_LOG(...)
#	define DEBUG_CHECK(...)
#else
#	include "log.h"
#	define DEBUG_LOG(...) log_add("DEBUG", __VA_ARGS__)
#	define DEBUG_CHECK(cond, ...) if(!(cond)) DEBUG_LOG(__VA_ARGS__);
#endif

#endif //DEBUG_H_

