#ifndef SPINLOCK_H_
#define SPINLOCK_H_

#include <atomic>
class spinlock{
private:
	std::atomic_flag state;

public:
	spinlock(void) { unlock(); }
	~spinlock(void) { unlock(); }

	void lock(void) {
		do continue; while(try_lock());
		while(!try_lock())
			continue;
	}
	bool try_lock(void) {
		return !state.test_and_set(std::memory_order_acquire);
	}
	void unlock(void) {
		state.clear(std::memory_order_release);
	}
};

#endif SPINLOCK_H_
