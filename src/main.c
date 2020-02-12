#include "config.h"
#include "def.h"
#include "log.h"
#include "mem.h"
#include "game.h"
#include "server.h"
#include "tibia_rsa.h"

#include <stdio.h>
#include <stdlib.h>

static void init_system(const char *name,
		bool(*init)(void), void(*shutdown)(void)){
	LOG("initializing `%s`...", name);
	if(!init()){
		// error messages are provided by
		// `init` in case of failure
		getchar(); exit(-1);
	}
	atexit(shutdown);
}

#ifdef BUILD_TEST
int kaplar_main(int argc, char **argv){
#else // BUILD_TEST
int main(int argc, char **argv){
#endif // BUILD_TEST

	// @TODO: maybe change `config` to `settings`
	config_cmdline(argc, argv);
	if(!config_load())
		LOG_WARNING("running with default config");

	// support systems
	init_system("mem", mem_init, mem_shutdown);
	init_system("tibia_rsa", tibia_rsa_init, tibia_rsa_shutdown);

	// network
	// @TODO: maybe change `server` to `net` or something else
	// @TODO: move these `add_protocol` into the server system
	svcmgr_add_protocol(&protocol_echo, config_geti("sv_echo_port"));
	svcmgr_add_protocol(&protocol_login, config_geti("sv_login_port"));
	init_system("server", server_init, server_shutdown);

	// game
	init_system("game", game_init, game_shutdown);

	game_run();
	return 0;
}
