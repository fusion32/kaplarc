#include "config.h"
#include "def.h"
#include "dispatcher.h"
#include "log.h"
#include "scheduler.h"
#include "system.h"

#include "db/db.h"
#include "server/server.h"
#include "server/server_rsa.h"

#include <stdio.h>
#include <stdlib.h>

static
void init_subsystem(const char *name, bool(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s` subsystem", name);
	if(!init()){
		// error messages should be
		// provided by the interface
		getchar(); exit(-1);
	}
	atexit(shutdown);
}

#ifdef BUILD_TEST
int kaplar_main(int argc, char **argv){
#else
int main(int argc, char **argv){
#endif // BUILD_TEST

	// parse command line and load config
	config_cmdline(argc, argv);
	if(!config_load())
		return -1;

	// initialize core interfaces
	//init_subsystem("dispatcher", dispatcher_init, dispatcher_shutdown);
	//init_subsystem("scheduler", scheduler_init, scheduler_shutdown);
	//init_subsystem("database", database_init, database_shutdown);

	// initialize server interfaces
	//init_subsystem("rsa", server_rsa_init, server_rsa_shutdown);
	

	// load server data
	// init game state

	// init server
	//svcmgr_add_protocol(&protocol_test, config_geti("sv_test_port"));
	init_subsystem("server", server_init, server_shutdown);
	getchar();
	return 0;
}
