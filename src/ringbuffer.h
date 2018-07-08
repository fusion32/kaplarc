#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "def.h"

template<typename T, size_t N>
class RingBuffer{
private:
	uint32 readpos;
	uint32 writepos;
	T buf[N];

	static constexpr int bitmask = (N - 1);
	static_assert(is_power_of_two(N),
		"Ringbuffer requires N to be a power of two.");
public:
	// delete copy and move operations
	RingBuffer(const RingBuffer&)			= delete;
	RingBuffer(RingBuffer&&)			= delete;
	RingBuffer &operator=(const RingBuffer&)	= delete;
	RingBuffer &operator=(RingBuffer&&)		= delete;

	RingBuffer(void) : readpos(0), writepos(0) {}
	~RingBuffer(void){}

	constexpr size_t capacity(void) const { return N; }
	uint32 size(void) const { return writepos - readpos; }
	bool empty(void) const { return writepos == readpos; }
	bool full(void) const { return size() >= N; }

	T &front(void){ return buf[readpos & bitmask]; }
	void pop(void){ if(!empty()) readpos++; }

	template<typename G>
	bool push(G &&item){
		if(full()) return false;
		buf[writepos++ & bitmask] = std::forward<G>(item);
		return true;
	}
};

#endif //RINGBUFFER_H_
