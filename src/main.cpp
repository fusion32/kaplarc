#include <vector>

#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "work.h"

int main(int argc, char **argv){
	work_init();
	scheduler_init();

	std::vector<SchRef> vec;
	vec.reserve(0x3FFF);
	for(int i = 0; i < 0x3FFF; ++i){
		vec.push_back(scheduler_add((i+1)*1000, [i](void){
			LOG("hello #%d", i);
		}));
	}

	//for(auto &ref : vec)
	//	scheduler_remove(ref);

	getchar();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}