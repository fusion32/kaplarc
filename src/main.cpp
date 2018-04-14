#include "config.h"
#include "db/cassandra.h"
#include "def.h"
#include "dispatcher.h"
#include "game/protocol_login.h"
#include "game/srsa.h"
#include "log.h"
#include "scheduler.h"
#include "server/protocol_test.h"
#include "server/server.h"
#include "system.h"

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
	// parse command line
	config_cmdline(argc, argv);

	// load config
	const char *config = config_get("config");
	if(!config){
		UNREACHABLE();
		return -1;
	}
	if(!config_load(config))
		return -1;

	// initialize core interfaces
	//init_interface("dispatcher", dispatcher_init, dispatcher_shutdown);
	init_interface("scheduler", scheduler_init, scheduler_shutdown);
	init_interface("cassandra backend", cass_init, cass_shutdown);

	// initialize game interfaces
	init_interface("RSA", srsa_init, srsa_shutdown);

	cass_test();
	getchar();

/*
	// initialize server
	server_add_protocol<ProtocolTest>(7171);
	server_add_protocol<ProtocolLogin>(config_geti("sv_login_port"));
	//server_add_protocol<ProtocolInfo>(config_geti("sv_info_port"));
	//server_add_protocol<ProtocolGame>(config_geti("sv_game_port"));
	server_run();
*/
	return 0;
}
