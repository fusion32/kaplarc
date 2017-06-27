#include "log.h"
#include "scheduler.h"
#include "work.h"
#include "system.h"
#include <stdio.h>

#include "avl_tree.h"

int main(int argc, char **argv)
{
	work_init();
	scheduler_init();

	kp::schref ref[4096];
	for(int i = 0; i < 4096; ++i){
		ref[i] = scheduler_add((i+1)*1000, [](void){
			LOG("hello");
		});
	}

	//for(int i = 0; i < 4096; ++i){
	//	scheduler_remove(ref[i]);
	//}

	getchar();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}