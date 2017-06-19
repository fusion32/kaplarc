#include "log.h"
#include "scheduler.h"
#include "work.h"
#include <stdio.h>

#include <list>

int main(int argc, char **argv)
{
	work_init();
	scheduler_init();

	kp::sch_entry entry[10];
	for(int i = 0; i < 10; ++i){
		entry[i] = scheduler_add((i+1)*1000, [](void){
			LOG("hello");
		});
	}

	for(int i = 0; i < 10; ++i){
		scheduler_pop(entry[i]);
	}

	getchar();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}
