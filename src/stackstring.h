#ifndef STACKSTRING_H_
#define STACKSTRING_H_

#include "stringbase.h"

template<int N>
class StackString : public StringBase{
private:
	char mem[N];

public:
	// default constructor
	StackString(void) : StringBase(N, 0, mem) {}

	// copy
	StackString(const StackString &str) : StringBase(N, 0, mem) { copy(str); }
	StackString(const char *str) : StringBase(N, 0, mem) { copy(str); }
	StackString &operator=(const StackString &str){ copy(str); }
	StackString &operator=(const char *str){ copy(str); }

	// move
	StackString(StackString&&) = delete;
	StackString &operator=(StackString&&) = delete;
};

#endif //STACKSTRING_H_
