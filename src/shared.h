#ifndef SHARED_H_
#define SHARED_H_

#include <atomic>

class Shared{
private:
	std::atomic<int> ref_count;

public:
	Shared(void) : ref_count(1) {}
	void incref(void){
		ref_count.fetch_add(1, std::memory_order_release);
	}
	void release(void){
		if(ref_count.fetch_sub(1, std::memory_order_acq_rel) <= 1)
			delete this;
	}
};

#endif //SHARED_H_