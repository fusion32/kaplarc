#ifndef LOG_H_
#define LOG_H_

#include <stdarg.h>

bool	log_start(void);
void	log_stop(void);

void	log_add(const char *tag, const char *fmt, ...);
void	log_add1(const char *tag, const char *fmt, va_list ap);

#define LOG(...)		log_add("INFO", __VA_ARGS__)
#define LOG_WARNING(...)	log_add("WARNING", __VA_ARGS__)
#define LOG_ERROR(...)		log_add("ERROR", __VA_ARGS__)

#ifndef _DEBUG
	#define LOG_DEBUG(...)
#else
	#define LOG_DEBUG(...) log_add("DEBUG", __VA_ARGS__)
#endif

#endif //LOG_H_
