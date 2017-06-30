#include "../../src/log.h"
#include "../../src/work.h"
#include "../../src/workgroup.h"

int main(int argc, char **argv)
{
	WorkGroup grp;

	// work
	auto f3 = [](void){
		LOG("func3");
	};
	auto f2 = [](void){
		LOG("func2");
	};
	auto f1 = [](void){
		LOG("func1");
	};

	// complete routines
	auto c2 = [](void){
		LOG("complete2");
		LOG("=======================");
	};
	auto c1 = [c2, &grp](void){
		LOG("complete1");
		LOG("=======================");
		grp.dispatch(c2);
	};

	// initialize worker threads
	work_init();

	// add work to group
	for(int i = 0; i < 5; i++){
		grp.add(f1);
		grp.add(f2);
		grp.add(f3);
	}

	// dispatch group
	grp.dispatch(c1);

	// cleanup
	getchar();
	work_shutdown();
	return 0;
}
