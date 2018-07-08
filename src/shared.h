#ifndef SHARED_H_
#define SHARED_H_

#include <atomic>

template<typename T>
class Shared{
private:
	std::atomic_int count_;

public:
	Shared(void) : count_{1} {}
	int count(void){
		return count_.load(std::memory_order_relaxed);
	}
	void incref(void){
		count_.fetch_add(1, std::memory_order_relaxed);
	}
	void decref(void){
		int prev_count = count_.fetch_sub(1,
			std::memory_order_relaxed);
		if(prev_count == 1)
			delete static_cast<T*>(this);
	}
};

#endif //SHARED_H_
