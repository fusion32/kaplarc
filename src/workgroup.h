#ifndef WORKGROUP_H_
#define WORKGROUP_H_

#include <atomic>
#include <vector>

#include "work.h"

class WorkGroup{
private:
	Work complete;
	std::vector<Work> vec;
	std::atomic<int> work_left;

public:
	WorkGroup(void){}
	WorkGroup(int count) : vec() { vec.reserve(count); }

	void add(Work wrk){
		vec.push_back(std::move(wrk));
	}

	void dispatch(Work complete_){
		complete = std::move(complete_);
		work_left.store((int)vec.size(), std::memory_order_release);
		for(auto &wrk : vec){
			work_dispatch([this, wrk](void) -> void {
				wrk();
				if(work_left.fetch_sub(1, std::memory_order_acq_rel) <= 1)
					complete();
			});
		}
	}
};

#endif //WORKGROUP_H_
