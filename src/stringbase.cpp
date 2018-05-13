#include "stringbase.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// string operations
void StringBase::append(const StringBase &str){
	int l = str.length;
	if(length + l >= capacity)
		l = capacity - length;
	if(l > 0){
		memcpy(buffer + length, str.buffer, l);
		length += l;
	}
}

void StringBase::append(const char *str){
	int l = (int)strlen(str);
	if(length + l >= capacity)
		l = capacity - length;
	if(l > 0){
		memcpy(buffer + length, str, l);
		length += l;
	}
}

void StringBase::copy(const StringBase &str){
	int l = str.length;
	if(l > capacity)
		l = capacity;
	if(l > 0){
		memcpy(buffer, str.buffer, l);
		length = l;
	}
}

void StringBase::copy(const char *str){
	int l = (int)strlen(str);
	if(l > capacity)
		l = capacity;
	if(l > 0){
		memcpy(buffer, str, l);
		length = l;
	}
}

// utility
void StringBase::appendf(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vappendf(fmt, ap);
	va_end(ap);
}

void StringBase::format(const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vformat(fmt, ap);
	va_end(ap);
}

void StringBase::vappendf(const char *fmt, va_list ap){
	int l;
	if(length >= capacity)
		return;
	l = vsnprintf(buffer + length,
		capacity - length, fmt, ap);
	if(l > 0)
		length += l;
}

void StringBase::vformat(const char *fmt, va_list ap){
	length = vsnprintf(buffer, capacity, fmt, ap);
	if(length < 0) length = 0;
}

