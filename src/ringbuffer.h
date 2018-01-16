#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <utility>

template<typename T, int N>
class RingBuffer{
private:
	int readpos;
	int writepos;
	int length;
	T buf[N];

public:
	// delete copy and move operations
	RingBuffer(const RingBuffer&)			= delete;
	RingBuffer(RingBuffer&&)			= delete;
	RingBuffer &operator=(const RingBuffer&)	= delete;
	RingBuffer &operator=(RingBuffer&&)		= delete;

	RingBuffer(void) : readpos(0), writepos(0), length(0) {}
	~RingBuffer(void){}

	int size(void) const { return length; }
	bool empty(void) const { return length == 0; }

	T &front(void){
		return buf[readpos];
	}

	void pop(void){
		if(length > 0){
			length--;
			readpos++;
			if(readpos >= N)
				readpos = 0;
		}
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

#endif //RINGBUFFER_H_
