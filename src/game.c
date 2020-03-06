#include "game.h"
//#include "db/db.h"
//#include "server/server.h"


struct ev_account_login{
	uint32 evtype;
	char accname[32];
	char password[32];
};

struct ev_account_login_resolve{
	uint32 evtype;

};

struct network_input_events{
	// account login
	struct mem_array ev_account_login;
	struct mem_array ev_account_login_resolve;
	// player login
	struct mem_array ev_player_login;
	struct mem_array ev_player_login_resolve;
	// player events
	// ...
};

// all input/output events are double buffered
static int ev_idx = 0;
static struct network_input_events net_in[2];

bool game_init(void){
	return true;
}

void game_shutdown(void){
}

/*
while(1){
	network messages are directly converted into events
	database results are directly converted into events
	convert expired scheduler entries to events

	process usual events (can generate output and script events)
	process script events (can generate output events)

	send net output events to server thread
	send db output events to server thread
}
*/

/* EVENT LIST
	DATABASE_IN:
		- db_result_account_login
		- db_result_player_login
		- ... (TODO)
	NETWORK_IN:
		- account_login
		- player_login
		- player_common_event: (1 byte identifier)
			- player_logout
			- player_ping
			- player_move_north
			- player_move_east
			- player_move_south
			- player_move_west
			- player_move_northeast
			- player_move_southeast
			- player_move_northwest
			- player_move_southwest
			- player_stop_autowalk
			- player_turn_north
			- player_turn_east
			- player_turn_south
			- player_turn_west
			- player_shop_close
			- player_trade_accept
			- player_trade_close
			- player_channels_request
			- player_npc_channel_close
			- player_party_leave
			- player_channel_create
			- player_cancel_move -> player_hold
			- player_outfit_request
			- player_questlog_request
		- player_autowalk
		- player_throw
		- player_shop_look
		- player_shop_purchase
		- player_shop_sell
		- player_trade_request
		- player_trade_look
		- player_item_use
		- player_item_use_ex
		- player_battle_window
		- player_item_rotate
		- player_container_close
		- player_container_uparrow
		- player_text_window -> player_item_text (?)
		- player_house_window
		- player_look
		- player_say
		- player_channel_open
		- player_channel_close
		- player_private_open
		- player_battle_mode
		- player_attack
		- player_follow
		- player_party_invite
		- player_party_join
		- player_party_invite_revoke
		- player_party_leadership_pass
		- player_party_shared_exp
		- player_channel_invite
		- player_channel_remove
		- player_tile_update_request
		- player_container_update_request
		- player_outfit_set
		- player_vip_add
		- player_vip_remove
		- player_questlog_request_one

		- CHECK:
			- player_bug_report
			- player_violation_window
			- player_violation_report
			- player_debug_assert
		- TODO:
			- player_mount

	DATABASE_OUT:
	NETWORK_OUT:
*/

void game_run(void){
	//for(ev in acc_login_requests):
	//	db_load_account(acc_login_resolve, ev.accname, ev.password)

	//for(ev in acc_login_resolve_events):
	//	acc_login_resolve(ev.data)
	//	add_connection_to_output_send

	// account login flow:
	// account_login (name, password) --> db_load_account (login_resolve, name, password)
	// db_result_account (login_resolve, loaded_info) --> login_resolve (loaded_info)
	// login_resolve --> network_send
}

