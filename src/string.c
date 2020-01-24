#include "string.h"
#include "def.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

// NOTE1: vsnprintf always insert the nul-terminator so the length
// of the string will have a maximum value of (capacity - 1)

// NOTE2: vsnprintf always return the number of characters (not
// counting the nul-terminator) that were or would be written
// to the string if it was big enough

void str_format(struct string_ptr *dst, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	str_vformat(dst, fmt, ap);
	va_end(ap);
}

void str_vformat(struct string_ptr *dst, const char *fmt, va_list ap){
	DEBUG_ASSERT(dst->cap > 0 &&
		"str_vappend: invalid string_ptr");
	int ret = vsnprintf(dst->data, dst->cap, fmt, ap);
	if(ret >= dst->cap)	dst->len = dst->cap - 1;
	else if(ret >= 0)	dst->len = ret;
	else			dst->len = 0;
}

void str_append(struct string_ptr *dst, const char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	str_vappend(dst, fmt, ap);
	va_end(ap);
}

void str_vappend(struct string_ptr *dst, const char *fmt, va_list ap){
	DEBUG_ASSERT(dst->cap > 0 && dst->cap > dst->len &&
		"str_vappend: invalid string_ptr");
	int ret;
	size_t cap;
	cap = dst->cap - dst->len;
	if(cap == 1) return;
	ret = vsnprintf(dst->data + dst->len, cap, fmt, ap);
	if(ret >= cap)		dst->len = dst->cap - 1;
	else if(ret > 0)	dst->len += ret;
}

void str_copy(struct string_ptr *dst, struct string_ptr *src){
	size_t dst_max_len = dst->cap - 1;
	size_t len = MIN(dst_max_len, src->len);
	memcpy(dst->data, src->data, len);
	dst->data[len] = 0;
	dst->len = len;
}

void str_strip_whitespace(struct string_ptr *str){
	int rdpos = 0;
	int wrpos = 0;
	bool last_white;
	bool cur_white;

	// skip leading whitespace
	while(rdpos < str->len && isblank(str->data[rdpos]))
		rdpos += 1;

	// skip duplicate whitespace
	for(last_white = false; rdpos < str->len; rdpos += 1){
		cur_white = isblank(str->data[rdpos]);
		if(cur_white && last_white)
			continue;
		last_white = cur_white;
		str->data[wrpos] = str->data[rdpos];
		wrpos += 1;
	}

	// skip trailing whitespace
	if(wrpos > 0){
		if(isblank(str->data[wrpos - 1]))
			wrpos -= 1;
	}

	// add nul-terminator and set new length
	str->data[wrpos] = 0;
	str->len = wrpos;
}
