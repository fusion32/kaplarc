#include <vector>

#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "work.h"

int main(int argc, char **argv){
	work_init();
	scheduler_init();

	getchar();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}