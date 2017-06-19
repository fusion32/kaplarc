#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <utility>

namespace kp{

template<typename T, int N>
class ringbuffer{
private:
	int readpos;
	int writepos;
	int length;
	T buf[N];

public:
	// delete copy and move operations
	ringbuffer(const ringbuffer &)			= delete;
	ringbuffer(ringbuffer &&)			= delete;
	ringbuffer &operator=(const ringbuffer &)	= delete;
	ringbuffer &operator=(ringbuffer &&)		= delete;

	ringbuffer() : readpos(0), writepos(0), length(0) {}
	~ringbuffer(){}

	int size() const { return length; }

	T *pop(){
		T *ptr = nullptr;
		if(length > 0){
			length--;
			ptr = &buf[readpos++];
			if(readpos >= N)
				readpos = 0;
		}
		return ptr;
	}

	template<typename G>
	bool push(G &&item){
		if(length >= N)
			return false;

		length++;
		buf[writepos++] = std::forward<G>(item);
		if(writepos >= N)
			writepos = 0;
		return true;
	}
};

} //namespace

#endif //RINGBUFFER_H_