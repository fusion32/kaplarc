#include "game.h"
//#include "db/db.h"
//#include "server/server.h"

bool game_init(void){
	return true;
}

void game_shutdown(void){
}

/*
NETWORK <-> GAME (I didn't include the database interaction but i'm thinking of something similar)

1 -	Network messages from both the login and game protocol are piped
	into the game input buffer (this is double buffered).

2 -	At the beggining of the game frame, the game input buffers are
	swapped and the buffer owned by the game thread is then parsed
	to update player state as well as issuing output and script events.

NOTE -	My previous idea was to have multiple of these double buffered
	input buffers (one per event type). It could improve the i-cache
	but it would worsen the d-cache plus the added complexity which
	would be overwhelming.

3 -	After parsing all input, the game state `ticks` (this could happen
	before but we need to have an order set) effectively updating the
	map, the monsters, the combat, etc etc. This will also issue output
	and script events.

4 -	Sort all script events by event type and execute them all. This may
	also issue output events.

5 -	Sort all output events and send them to the network thread (somehow,
	still need thinking).
*/

/* EVENT LIST
	DATABASE_OUT:
	NETWORK_OUT:
		- TODO

	DATABASE_IN:
		- db_result_account_login
		- db_result_player_login
		- ... (TODO)

	NETWORK_IN:
	(lets assume for now that a string is a nul-terminated
	allocated string so we'll can use a single pointer)
		- account_login (16 bytes)
			* string: accname
			* string: password
		- player_login (24 bytes)
			* string: accname
			* string: password
			* string: charname
		- player_common_event (u8: id) (1 byte)
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
		- player_autowalk (up to 256 bytes)
			* 1x u8: path_length
			* u8[path_length]: path
		- player_throw (14 bytes)
			* u16[2] + u8: from position
			* u16: sprite id
			* u8: from stackpos
			* u16[2] + u8: to position
			* u8: item count
		- player_shop_look (3 bytes)
			* u16: sprite id
			* u8: subtype (it can be the fluid type, item count, rune count, ...)
		- player_shop_purchase (6 bytes)
			* u16: sprite id
			* u8: subtype
			* u8: amount
			* u8: ignore capacity
			* u8: buy with backpack
		- player_shop_sell (4 bytes)
			* u16: sprite id
			* u8: subtype
			* u8: amount
		- player_trade_request (12 bytes)
			* u16[2] + u8: position
			* u16: sprite id
			* u8: stackpos
			* u32: player id
		- player_trade_look (2 bytes)
			* u8: counter offer
			* u8: index
		- player_item_use (9 bytes)
			* u16[2] + u8: position
			* u16: sprite id
			* u8: stackpos
			* u8: index
			** (is_hotkey == (pos.x == 0xFFFF && pos.y == 0 && pos.z == 0))
		- player_item_use_ex (16 bytes)
			* u16[2] + u8: from position
			* u16: from sprite id
			* u8: from stackpos
			* u16[2] + u8: to position
			* u16: to sprite id
			* u8: to stackpos
			** (is_hotkey == (from_pos.x == 0xFFFF && from_pos.y == 0 && from_pos.z == 0))
		- player_battle_window (12 bytes)
			* u16[2] + u8: position
			* u16: sprite id
			* u8: stackpos
			* u32: creature id
			** (is_hotkey == (pos.x == 0xFFFF && pos.y == 0 && pos.z == 0))
		- player_item_rotate (8 bytes)
			* u16[2] + u8: position
			* u16: sprite id
			* u8: stackpos
		- player_container_close (1 byte)
			* u8: container id
		- player_container_uparrow (1 byte)
			* u8: container id
		- player_container_update_request (1 byte)
			* u8: container id
		- player_text_window -> player_item_text (?) (12 bytes)
			* u32: window text id
			* string: new text
		- player_house_window (13 bytes)
			* u8: door id
			* u32: window text id (?)
			* string: text
		- player_look (8 bytes)
			* u16[2] + u8: position
			* u16: sprite id
			* u8: stackpos
		- player_say (up to 17 bytes)
			* u8: type
			* if type == private: string: receiver
			* if type == channel: u16: channel_id
			* string: text
		- player_channel_open (2 bytes)
			* u16: channel id
		- player_channel_close (2 bytes)
			* u16: channel id
		- player_private_open (8 bytes)
			* string: receiver
		- player_battle_mode (3 bytes)
			* u8: fight mode
			* u8: chase mode
			* u8: safe mode
		- player_attack (12 bytes)
			* u32: creature id
			* u32: ?
			* u32: ?
		- player_follow (4 bytes)
			* u32: creature id
		- player_party_invite (4 bytes)
			* u32: creature id
		- player_party_join (4 bytes)
			* u32: creature id
		- player_party_invite_revoke (4 bytes)
			* u32: creature id
		- player_party_leadership_pass (4 bytes)
			* u32: creature id
		- player_party_shared_exp (2 bytes)
			* u8: shared exp
			* u8: ?
		- player_channel_invite (8 bytes)
			* string: name
		- player_channel_remove (8 bytes)
			* string: name
		- player_tile_update_request (5 bytes)
			* u16[2] + u8: position
		- player_outfit_set (9 bytes)
			* u16: looktype
			* u8: lookhead
			* u8: lookbody
			* u8: looklegs
			* u8: lookfeet
			* u8: lookaddons
			* u16: lookmount
		- player_vip_add (8 bytes)
			* string: name
		- player_vip_remove (4 bytes)
			* u32: player id
		- player_questlog_request_one (2 bytes)
			* u16: quest id
		- TODO:
			- player_mount
			- player_bug_report
			- player_violation_window
			- player_violation_report
			- player_debug_assert

	SORTED EVENT LIST (size in parenthesis doesn't account for the 1 byte identifier)

		- player_autowalk (up to 256 bytes)
		- player_login (24 bytes)
		- player_say (up to 17 bytes)
		- account_login (16 bytes)

		- player_item_use_ex (16 bytes)
		- player_throw (14 bytes)
		- player_house_window (13 bytes)
		- player_trade_request (12 bytes)
		- player_battle_window (12 bytes)
		- player_text_window -> player_item_text (?) (12 bytes)
		- player_attack (12 bytes)
		- player_item_use (9 bytes)
		- player_outfit_set (9 bytes)

		- player_item_rotate (8 bytes)
		- player_look (8 bytes)
		- player_private_open (8 bytes)
		- player_channel_invite (8 bytes)
		- player_channel_remove (8 bytes)
		- player_vip_add (8 bytes)
		- player_shop_purchase (6 bytes)
		- player_tile_update_request (5 bytes)

		- player_shop_sell (4 bytes)
		- player_follow (4 bytes)
		- player_party_invite (4 bytes)
		- player_party_join (4 bytes)
		- player_party_invite_revoke (4 bytes)
		- player_party_leadership_pass (4 bytes)
		- player_vip_remove (4 bytes)
		- player_shop_look (3 bytes)
		- player_battle_mode (3 bytes)
		- player_trade_look (2 bytes)
		- player_channel_open (2 bytes)
		- player_channel_close (2 bytes)
		- player_party_shared_exp (2 bytes)
		- player_questlog_request_one (2 bytes)
		- player_container_close (1 byte)
		- player_container_uparrow (1 byte)
		- player_container_update_request (1 byte)

		- player_common_event (0 bytes)
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

	//
}

