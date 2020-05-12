#include "config.h"
#include "def.h"
#include "log.h"
#include "game.h"
#include "tibia_rsa.h"

#include "server.h"
#include "db/pgsql.h"

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

	config_init(argc, argv);
	if(!config_load())
		LOG_WARNING("running with default config");

	// support systems
	init_system("mem", mem_init, mem_shutdown);
	init_system("pgsql", pgsql_init, pgsql_shutdown);
	init_system("tibia_rsa", tibia_rsa_init, tibia_rsa_shutdown);

	// network
	extern struct protocol protocol_echo;
	//extern struct protocol protocol_login;
	svcmgr_add_protocol(&protocol_echo, config_geti("sv_echo_port"));
	//svcmgr_add_protocol(&protocol_login, config_geti("sv_login_port"));
	init_system("server", server_init, server_shutdown);

	// game
	init_system("game", game_init, game_shutdown);

	//game_run();
	struct db_result_account_login acc;
	ASSERT(pgsql_load_account_login("admin", &acc));
	LOG("acc: {'%s', '%s', '%llu', '%s'}",
		"admin", acc.password,
		acc.premend, acc.charlist);
	getchar();
	return 0;
}
