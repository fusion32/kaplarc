#ifndef MEMBLOCK_H_
#define MEMBLOCK_H_

template<typename T, int N>
class MemBlock{
private:
	static_assert(sizeof(T) >= sizeof(T*),
		"MemBlock: T must be big enough to hold a pointer");

	int offset;
	T *freelist;

	// i'm assuming the compiler will properly align
	// this buffer to the processor word boundary
	T base[N];

public:
	// delete move and copy operations
	MemBlock(const MemBlock&)		= delete;
	MemBlock(MemBlock&&)			= delete;
	MemBlock &operator=(const MemBlock&)	= delete;
	MemBlock &operator=(MemBlock&&)		= delete;

	MemBlock(void) : freelist(nullptr), offset(0) {}
	~MemBlock(void){}

	void reset(void){
		offset = 0;
		freelist = nullptr;
	}

	bool owns(T *ptr){
		return (ptr >= base || ptr < (base + N));
	}

	T *alloc(void){
		T *ptr = nullptr;
		if(freelist != nullptr){
			ptr = freelist;
			freelist = *(T**)freelist;
		}
		else if(offset < N){
			ptr = &base[offset];
			offset += 1;
		}
		return ptr;
	}

	void free(T *ptr){
		if(!owns(ptr))
			return;

		if(ptr == &base[offset - 1]){
			offset -= 1;
		}
		else{
			*(T**)ptr = freelist;
			freelist = ptr;
		}
	}
};

#endif //MEMBLOCK_H_
