#ifndef HEAPSTRING_H_
#define HEAPSTRING_H_

#include "stringbase.h"

class HeapString : public StringBase {
public:
	// destructor
	~HeapString(void);

	// copy
	HeapString(const HeapString &str);
	HeapString(const char *str);
	HeapString &operator=(const HeapString &str);
	HeapString &operator=(const char *str);

	// move
	HeapString(HeapString &&str);
	HeapString &operator=(HeapString &&str);

	// dynamic functionality
	void resize(size_t s);
	void expand(size_t s);
	void shrink(void);
};

#endif //HEAPSTRING_H_
