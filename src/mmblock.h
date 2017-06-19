#ifndef MMBLOCK_H_
#define MMBLOCK_H_

namespace kpl{

template<typename T, int N>
class mmblock{
private:
	static_assert(sizeof(T) >= sizeof(T*),
		"mmblock: T must be big enough to hold a pointer");

	int offset;
	T *freelist;

	// i'm assuming the compiler will properly align
	// this buffer to the word boundary
	T base[N];

public:
	// delete move and copy operations
	mmblock(const mmblock &blk)		= delete;
	mmblock &operator=(const mmblock &blk)	= delete;
	mmblock(mmblock &&blk)			= delete;
	mmblock &operator=(mmblock &&blk)	= delete;

	mmblock(void) : freelist(nullptr), offset(0) {}
	~mmblock(void){}

	void reset(void){
		offset = 0;
		freelist = nullptr;
	}

	bool owns(T *ptr){
		return (ptr >= base || ptr <= (base + N - 1));
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

} //namespace

#endif //MMBLOCK_H_
