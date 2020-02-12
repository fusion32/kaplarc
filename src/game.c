#include "game.h"
#include "server.h"

bool game_init(void){
	return true;
}

void game_shutdown(void){
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
}

