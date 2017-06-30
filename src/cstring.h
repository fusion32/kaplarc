#ifndef CSTRING_H_
#define CSTRING_H_

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

template<int N>
class CString{
private:
	int length;
	char buf[N];

	friend class CString;

public:
	// delete move operations
	CString(CString &&str)			= delete;
	CString &operator=(CString &&str)	= delete;

	// constructors
	CString(void) : length(0) {}
	template<int M>
	CString(const CString<M> &str){ copy(str); }
	CString(const char *str){ copy(str); }

	// destructor
	~CString(void){}

	// string operations
	void vformat(const char *fmt, va_list ap){
		if(length >= N)
			return;
		length = vsnprintf(buf, N, fmt, ap);
		if(length < 0)
			length = 0;
	}

	void format(const char *fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vformat(fmt, ap);
		va_end(ap);
	}

	void vformat_append(const char *fmt, va_list ap){
		int len;
		if(length >= N)
			return;
		len = vsnprintf(buf + length, N - length, fmt, ap);
		if(len > 0)
			length += len;
	}

	void format_append(const char *fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vformat_append(fmt, ap);
		va_end(ap);
	}

	template<int M>
	void append(const CString<M> &str){
		int len = str.length;
		if(len + length >= N - 1)
			len = N - 1 - length;
		if(len > 0){
			memcpy(buf + length, str.buf, len);
			length += len;
		}
	}

	void append(const char *str){
		int len = (int)strlen(str);
		if(len >= N - 1 - length)
			len = N - 1 - length;
		if(len > 0){
			memcpy(buf + length, str, len);
			length += len;
		}
	}

	template<int M>
	void copy(const CString<M> &str){
		int len = str.length;
		if(len > N - 1)
			len = N - 1;
		if(len > 0){
			memcpy(buf, str.buf, len);
			length = len;
		}
	}

	void copy(const char *str){
		int len = (int)strlen(str);
		if(len > N - 1)
			len = N - 1;
		if(len > 0){
			memcpy(buf, str, len);
			length = len;
		}
	}

	int size(void) const{
		return length;
	}

	// convert to cstr
	const char *cstr(void){
		buf[length] = 0x00;
		return buf;
	}

	operator const char*(void){
		return cstr();
	}

	// operators overload
	template<int M>
	CString &operator=(const CString<M> &str){
		copy(str);
		return *this;
	}

	CString &operator=(const char *str){
		copy(str);
		return *this;
	}

	template<int M>
	CString &operator+=(const CString<M> &str){
		append(str);
		return *this;
	}

	CString &operator+=(const char *str){
		append(str);
		return *this;
	}

	template<int M>
	CString &operator+(const CString<M> &str){
		append(str);
		return *this;
	}

	CString &operator+(const char *str){
		append(str);
		return *this;
	}
};

#endif //CSTRING_H_

