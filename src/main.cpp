#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "dispatcher.h"

#include "server/server.h"
#include "server/protocol_test.h"

#include <stdlib.h>

static
void init_interface(const char *name, void(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s` interface", name);
	init(); atexit(shutdown);
}

static
void init_interface(const char *name, bool(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s` interface", name);
	if(!init()){
		// error messages should be
		// provided by the interface
		getchar(); exit(-1);
	}
	atexit(shutdown);
}

int main(int argc, char **argv){
	// initialize core interfaces
	init_interface("dispatcher", dispatcher_init, dispatcher_shutdown);
	init_interface("scheduler", scheduler_init, scheduler_shutdown);

	scheduler_add(5000, [](void){
		LOG("hello");
	});

	for(int i = 0; i < 1000; i++)
		scheduler_add(i * 1000,
			[i](void){ LOG("%d", i); });

	server_add_protocol<ProtocolTest>(7171);
	server_run();
	return 0;
}
