#include "../../src/env.h"
#include "../../src/log.h"

int main(int argc, char **argv)
{
	int req[4];

	// worker doesn't need any other interface
	env_set(ENV_WORKER);
	// init worker

	// scheduler requires the scheduler
	req[0] = ENV_WORKER;
	if(env_lock(req, 1)){
		env_set(ENV_SCHEDULER);
		// init scheduler
	}else{
		LOG("failed to initialize scheduler");
		return -1;
	}

	// server requires the worker and the scheduler
	req[1] = ENV_SCHEDULER;
	if(env_lock(req, 2)){
		env_set(ENV_SERVER);
		// init server
	}else{
		LOG("failed to initialize server");
		return -1;
	}

	LOG("server running...");
	return 0;
}
