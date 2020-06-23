#ifndef LOG_H_
#define LOG_H_

#include "def.h"

void log_to_file(void);
void log_to_stdout(void);

void log_add(const char *tag, const char *fmt, ...);
void log_add1(const char *tag, const char *fmt, va_list ap);

#define LOG(...)		log_add("INFO", __VA_ARGS__)
#define LOG_WARNING(...)	log_add("WARNING", __VA_ARGS__)
#define LOG_ERROR(...)		log_add("ERROR", __VA_ARGS__)

#endif //LOG_H_
