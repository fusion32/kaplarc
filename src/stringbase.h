#ifndef STRINGBASE_H_
#define STRINGBASE_H_

#include <stddef.h>
#include <stdarg.h>

class StringBase{
protected:
	size_t capacity_;
	size_t length_;
	char *buffer_;

public:
	// constructors
	StringBase(void) : capacity_(0), length_(0), buffer_(nullptr) {}
	StringBase(size_t cap, size_t len, char *buf)
		: capacity_(cap), length_(len), buffer_(buf) {}

	// destructor
	~StringBase(void) {}

	// info
	bool empty(void) { return length_ == 0; }
	size_t size(void) { return length_; }
	const char *ptr(void) { return buffer_; }

	// conversion
	const char *c_str(void){
		if(length_ == capacity_)
			length_ -= 1;
		buffer_[length_] = 0x00;
		return buffer_;
	}
	operator const char*(void){
		return c_str();
	}

	// string operations
	void append(const StringBase &str);
	void append(const char *str);
	void copy(const StringBase &str);
	void copy(const char *str);

	// utility
	void appendf(const char *fmt, ...);
	void format(const char *fmt, ...);
	void vappendf(const char *fmt, va_list ap);
	void vformat(const char *fmt, va_list ap);

};

#endif //STRINGBASE_H_

