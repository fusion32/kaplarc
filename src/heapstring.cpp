#include "def.h"
#include "heapstring.h"
#include <string.h>
#include <stdlib.h>

#define ROUND_TO_32(x) (((x) + 31) & ~31)

// destructor
HeapString::~HeapString(void){
	resize(0);
}

// copy
HeapString::HeapString(const HeapString &str){
	resize(str.capacity_);
	copy(str);
}
HeapString::HeapString(const char *str){
	resize(strlen(str));
	copy(str);
}
HeapString &HeapString::operator=(const HeapString &str){
	resize(str.capacity_);
	copy(str);
	return *this;
}
HeapString &HeapString::operator=(const char *str){
	resize(strlen(str));
	copy(str);
	return *this;
}

// move
HeapString::HeapString(HeapString &&str)
 : StringBase(str.capacity_, str.length_, str.buffer_){
	str.capacity_ = 0;
	str.length_ = 0;
	str.buffer_ = nullptr;
}
HeapString &HeapString::operator=(HeapString &&str){
	resize(0); // free own buffer
	capacity_ = str.capacity_;
	length_ = str.length_;
	buffer_ = str.buffer_;
	str.capacity_ = 0;
	str.length_ = 0;
	str.buffer_ = nullptr;
	return *this;
}

// dynamic functionality
void HeapString::resize(size_t s){
	if(s != 0){
		s = ROUND_TO_32(s);
		if(buffer_ == nullptr)
			buffer_ = (char*)::malloc(s);
		else
			buffer_ = (char*)::realloc(buffer_, s);
		if(buffer_ == nullptr)
			UNREACHABLE();
		capacity_ = s;
	}else{
		capacity_ = 0;
		length_ = 0;
		if(buffer_ != nullptr){
			::free(buffer_);
			buffer_ = nullptr;
		}
	}
}
void HeapString::expand(size_t s){
	s += capacity_;
	resize(s);
}
void HeapString::shrink(void){
	size_t s = length_;
	s = ROUND_TO_32(s);
	if(s < capacity_)
		resize(s);
}
