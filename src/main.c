#include "config.h"
#include "common.h"
#include "log.h"
#include "game.h"
#include "outbuf.h"
#include "tibia_rsa.h"

#include "server/server.h"
#include "db/database.h"

#include <stdio.h>
#include <stdlib.h>

static void
init_system(const char *name,
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
int kpl_main(int argc, char **argv){
#else // BUILD_TEST
int main(int argc, char **argv){
#endif // BUILD_TEST

	config_init(argc, argv);
	if(!config_load())
		LOG_WARNING("running with default config");

	// init support systems
	init_system("outbuf", outbuf_init, outbuf_shutdown);
	init_system("tibia_rsa", tibia_rsa_init, tibia_rsa_shutdown);

	// init database thread
	init_system("database", db_init, db_shutdown);

	// init network thread
	extern struct protocol protocol_echo;
	extern struct protocol protocol_login;
	extern struct protocol protocol_game;
	svcmgr_add_protocol(&protocol_echo, config_geti("sv_echo_port"));
	svcmgr_add_protocol(&protocol_login, config_geti("sv_login_port"));
	svcmgr_add_protocol(&protocol_game, config_geti("sv_game_port"));
	init_system("server", server_init, server_shutdown);

	// init and run game thread
	init_system("game", game_init, game_shutdown);
	LOG("server running...");
	game_run();
	return 0;
}
