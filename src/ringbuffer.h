#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include "def.h"

template<typename T, uint32 N>
class RingBuffer{
private:
	uint32 readpos;
	uint32 writepos;
	T buf[N];

	static constexpr int clamp_mask = (N - 1);
	static_assert((N != 0) && ((N & (N - 1)) == 0),
		"Ringbuffer requires N to be a power of 2.");
public:
	// delete copy and move operations
	RingBuffer(const RingBuffer&)			= delete;
	RingBuffer(RingBuffer&&)			= delete;
	RingBuffer &operator=(const RingBuffer&)	= delete;
	RingBuffer &operator=(RingBuffer&&)		= delete;

	RingBuffer(void) : readpos(0), writepos(0) {}
	~RingBuffer(void){}

	constexpr uint32 capacity(void) const { return N; }
	uint32 size(void) const { return writepos - readpos; }
	bool empty(void) const { return writepos == readpos; }
	bool full(void) const { return size() >= N; }

	T &front(void){ return buf[readpos & clamp_mask]; }
	void pop(void){ if(!empty()) readpos++; }

	template<typename G>
	bool push(G &&item){
		if(full()) return false;
		buf[writepos++ & clamp_mask] = std::forward<G>(item);
		return true;
	}
};

#endif //RINGBUFFER_H_
