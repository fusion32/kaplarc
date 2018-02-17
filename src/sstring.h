#ifndef SSTRING_H_
#define SSTRING_H_

#include "stringbase.h"

template<int N>
class SString : public StringBase{
private:
	char mem[N];
public:
	SString(void) : StringBase(N, 0, mem) {}
	SString(const SString &str) : StringBase(N, 0, mem) { copy(str); }
	SString(const char *str) : StringBase(N, 0, mem) { copy(str); }
};

#endif //SSTRING_H_
