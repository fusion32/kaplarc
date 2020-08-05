/*

  Server Diagram:
   ____________    ___________
  |            |  |           |
  | NET THREAD |  | DB THREAD |
  |____________|  |___________|
      /\  ||          /\  ||   |-> IO DATA EXCHANGE
      ||  \/          ||  \/   |
   ___________________________
  |                           |
  |       GAME THREAD         |
  |___________________________|
            /\    ||
            ||    \/
   ___________________________
  |                           |
  |      WORKER THREADS       |
  |___________________________|

*/

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
int kaplar_main(int argc, char **argv){
#else // BUILD_TEST
int main(int argc, char **argv){
#endif // BUILD_TEST

	config_init(argc, argv);
	if(!config_load())
		LOG_WARNING("running with default config");

	// init support systems
	//init_system("mem", mem_init, mem_shutdown);
	init_system("outbuf", outbuf_init, outbuf_shutdown);
	init_system("tibia_rsa", tibia_rsa_init, tibia_rsa_shutdown);

	// init database thread
	// @TODO

	// init network thread
	extern struct protocol protocol_echo;
	extern struct protocol protocol_login;
	svcmgr_add_protocol(&protocol_echo, config_geti("sv_echo_port"));
	svcmgr_add_protocol(&protocol_login, config_geti("sv_login_port"));
	init_system("server", server_init, server_shutdown);

	// init worker threads
	// @TODO

#if 0
	game_run();
#else
	init_system("database", db_init, db_shutdown);
	LOG("====================");
	db_print_account("admin");
	LOG("====================");
	db_print_account("acctest");
	LOG("====================");
	db_print_account("emptytest");
	getchar();
#endif
	return 0;
}
