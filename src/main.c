#include "config.h"
#include "def.h"
#include "dispatcher.h"
#include "log.h"
#include "scheduler.h"
#include "system.h"

#include "db/db.h"
#include "server/protocol_test.h"
#include "server/server.h"
#include "server/server_rsa.h"

#include <stdio.h>
#include <stdlib.h>

static
void init_interface(const char *name, void(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s` interface", name);
	init(); atexit(shutdown);
}

static
void init_interface_may_fail(const char *name, bool(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s` interface", name);
	if(!init()){
		// error messages should be
		// provided by the interface
		getchar(); exit(-1);
	}
	atexit(shutdown);
}

int main1(int argc, char **argv){
	// parse command line and load config
	config_cmdline(argc, argv);
	if(!config_load())
		return -1;

	// initialize core interfaces
	init_interface("dispatcher", dispatcher_init, dispatcher_shutdown);
	init_interface("scheduler", scheduler_init, scheduler_shutdown);
	//init_interface("database", database_init, database_shutdown);

	// initialize server interfaces
	init_interface_may_fail("RSA", server_rsa_init, server_rsa_shutdown);

	// initialize cluster protocols
	// TODO

	// load server data
	// init game state

	// init server
	//server_add_protocol(&protocol_test, config_geti("sv_test_port"));
	//server_run();
	return 0;
}