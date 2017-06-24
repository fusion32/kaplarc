#include "log.h"
#include "scheduler.h"
#include "work.h"
#include "system.h"
#include <stdio.h>

#include "avl_tree.h"

int main(int argc, char **argv)
{
	kp::avl_tree<int, 256> tree;
	LOG("inserting");
	for(int i = 0; i < 64; i++){
		LOG("inserting %d", (i + 1337) % 256);
		tree.insert((i + 1337) % 256);
	}
	LOG("ok");

	tree.print();
	getchar();
	return 0;

/*
	work_init();
	scheduler_init();

	kp::sch_entry *entry[4096];
	for(int i = 0; i < 4096; ++i){
		entry[i] = scheduler_add((i+1)*1000, [](void){
			LOG("hello");
		});
	}

	for(int i = 0; i < 4096; ++i){
		scheduler_pop(entry[i]);
	}

	getchar();
	scheduler_shutdown();
	work_shutdown();
	return 0;
*/
}
