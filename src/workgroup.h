#ifndef WORKGROUP_H_
#define WORKGROUP_H_

#include "work.h"

#include <atomic>
#include <vector>

namespace kp{

class workgroup{
private:
	std::vector<kp::work> vec;
	std::atomic<int> work_left;

public:
	workgroup(void){}
	workgroup(int count) : vec(count) {}
	~workgroup(void){}

	void add(kp::work wrk){
		vec.push_back(std::move(wrk));
	}

	void dispatch(kp::work complete){
		work_left.store((int)vec.size(), std::memory_order_release);
		for(auto &wrk : vec){
			work_dispatch([wrk, complete, this](void) -> void {
				wrk();
				if(work_left.fetch_sub(1, std::memory_order_acq_rel) <= 1)
					complete();
			});
		}
	}
};

} //namespace

#endif //WORKGROUP_H_
