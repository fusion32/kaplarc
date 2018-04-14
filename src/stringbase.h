#ifndef STRINGBASE_H_
#define STRINGBASE_H_

#include <stdarg.h>

class StringBase{
protected:
	int capacity;
	int length;
	char *buffer;

public:
	// constructors
	StringBase(void) : capacity(0), length(0), buffer(nullptr) {}
	StringBase(int capacity_, int length_, char *buffer_)
		: capacity(capacity_), length(length_), buffer(buffer_) {}

	// destructor
	~StringBase(void) {}

	// info
	bool empty(void) { return length == 0; }
	int size(void) { return length; }
	const char *str(void) { return buffer; }

	// conversion
	const char *c_str(void){
		buffer[length] = 0x00;
		return buffer;
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
	void copyf(const char *fmt, ...);
	void vappendf(const char *fmt, va_list ap);
	void vcopyf(const char *fmt, va_list ap);

};

#endif //STRINGBASE_H_

