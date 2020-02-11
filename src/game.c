#include "game.h"
#include "tibia_rsa.h"
#include "server.h"

static struct {
	const char *name;
	bool (*init)(void);
	void (*shutdown)(void);
} subsystems[] = {
	{"tibia_rsa", tibia_rsa_init, tibia_rsa_shutdown},
};

bool game_init(void){
	int i, num_subsystems;
	num_subsystems = ARRAY_SIZE(subsystems);
	for(i = 0; i < num_subsystems; i += 1){
		LOG("initializing game subsystem `%s`...",subsystems[i].name);
		if(!subsystems[i].init()){
			LOG_ERROR("failed");
			goto fail;
		}
	}
	return true;

fail:	for(i -= 1; i >= 0; i -= 1)
		subsystems[i].shutdown();
	return false;
}

void game_shutdown(void){
	int i = ARRAY_SIZE(subsystems) - 1;
	while(i >= 0){
		subsystems[i].shutdown();
		i -= 1;
	}
}

#include <stdio.h>
void game_run(void){
	//while(1){
		// convert server net input to events (this will come from dispatching work into the game thread)
		// convert scheduler entries to events (if they have expired)
		// process chat events
		// process combat events
		// process move events
		// process item events (use/add/remove items)
		// ... player, npc, monster, all events ...
		// process all events (these may also generate script events)
		// process script events (script callbacks)
		// update whatever is left
		// send output commands to server thread
	//}
	getchar();

	while(1){
		//server_exec(game_dispatch_to_server, NULL);
	}
}

