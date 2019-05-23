#include "stringbase.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// string operations
void StringBase::append(const StringBase &str){
	size_t l = str.length_;
	if(length_ + l >= capacity_)
		l = capacity_ - length_;
	if(l > 0){
		memcpy(buffer_ + length_, str.buffer_, l);
		length_ += l;
	}
}

void StringBase::append(const char *str){
	size_t l = strlen(str);
	if(length_ + l >= capacity_)
		l = capacity_ - length_;
	if(l > 0){
		memcpy(buffer_ + length_, str, l);
		length_ += l;
	}
}

void StringBase::copy(const StringBase &str){
	size_t l = str.length_;
	if(l > capacity_)
		l = capacity_;
	if(l > 0){
		memcpy(buffer_, str.buffer_, l);
		length_ = l;
	}
}

void StringBase::copy(const char *str){
	size_t l = strlen(str);
	if(l > capacity_)
		l = capacity_;
	if(l > 0){
		memcpy(buffer_, str, l);
		length_ = l;
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
	int ret;
	size_t cap;
	if(length_ >= capacity_)
		return;
	cap = capacity_ - length_;
	ret = vsnprintf(buffer_ + length_, cap, fmt, ap);
	if(ret > cap)		length_ = capacity_;
	else if(ret > 0)	length_ += ret;
}

void StringBase::vformat(const char *fmt, va_list ap){
	int ret = vsnprintf(buffer_, capacity_, fmt, ap);
	if(ret > capacity_)	length_ = capacity_;
	else if(ret < 0)	length_ = 0;
	else			length_ = ret;
}

