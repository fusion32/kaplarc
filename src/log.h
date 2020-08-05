#ifndef KAPLAR_LOG_H_
#define KAPLAR_LOG_H_ 1

#include "common.h"

void log_to_file(void);
void log_to_stdout(void);

void log_add(const char *tag, const char *fmt, ...);
void log_addv(const char *tag, const char *fmt, va_list ap);

#define LOG(...)		log_add("INFO", __VA_ARGS__)
#define LOG_WARNING(...)	log_add("WARNING", __VA_ARGS__)
#define LOG_ERROR(...)		log_add("ERROR", __VA_ARGS__)

#endif //KAPLAR_LOG_H_
