#ifndef STRINGUTIL_H_
#define STRINGUTIL_H_

#include <stdarg.h>

struct string_ptr{
	size_t cap;
	size_t len;
	char *data;
};

void str_format(struct string_ptr *dst, const char *fmt, ...);
void str_vformat(struct string_ptr *dst, const char *fmt, va_list ap);
void str_append(struct string_ptr *dst, const char *fmt, ...);
void str_vappend(struct string_ptr *dst, const char *fmt, va_list ap);

void str_copy(struct string_ptr *dst, struct string_ptr *src);
void str_strip_whitespace(struct string_ptr *str);

#endif //STRINGUTIL_H_
