#include "game.h"
#include "buffer_util.h"
#include "server/server.h"

// @REVISIT:
//	- should the input buffer size be in the config?
//	(would require dynamic allocation)
//	- should the input buffer grow (in blocks)?

/* LOGIN/PLAYER INPUT (NET -> GAME) */
#define NET_INPUT_BUFFER_SIZE (4 * 1024 * 1024) // 4MB
static int net_idx = 0;
static uint32 net_input_bufptr[2];
static uint8 net_input_buffer[2][NET_INPUT_BUFFER_SIZE];

/* LOGIN REQUESTS */
#include "thread.h"
struct login_request{
	uint32 connection;
	uint32 xtea[4];
	char accname[32];
	char password[32];
};


//	Input that contain strings or any variable length array should
// be checked for consistency before being added to the input buffer
// or it could happen that an artificial message could break something.

//	INPUT LAYOUT
//		u16: input length
//		u16: command
//		u32: command handle (eg: connection for login commands, player for player commands, etc...)
//		u8 x input length: command data

// THIS SHOULD BE CALLED FROM THE NETWORK THREAD ONLY
bool game_add_net_input(uint16 command, uint32 command_handle, uint8 *data, uint16 datalen){
	uint8 *buffer;
	int idx = net_idx;
	uint32 bufptr = net_input_bufptr[idx];
	uint16 input_len = 8 + datalen;
	if((bufptr + input_len) >= NET_INPUT_BUFFER_SIZE){
		LOG_ERROR("game_add_net_input: net input buffer overflow");
		return false;
	}
	// encode data into the current input buffer
	buffer = &net_input_buffer[idx][bufptr];
	encode_u16_le(buffer + 0, input_len);
	encode_u16_le(buffer + 2, command);
	encode_u32_le(buffer + 4, command_handle);
	memcpy(buffer + 8, data, datalen);
	// advance the current buffer pointer
	net_input_bufptr[idx] += input_len;
	return true;
}

#if 1
static INLINE uint16 decode_tibia_string(uint8 *data, char *outstr, uint16 maxlen){
	uint16 len = decode_u16_le(data);
	uint16 copy_len;
	DEBUG_ASSERT(maxlen > 0);
	//if(maxlen > 0){
		if(len > 0){
			copy_len = len >= maxlen ? maxlen - 1 : len;
			memcpy(outstr, data + 2, copy_len);
			outstr[copy_len] = 0;
		}else{
			outstr[0] = 0;
		}
	//}
	return len + 2;
}
#endif

static void game_consume_net_input(void){
	int idx = 1 - net_idx;
	uint32 len = net_input_bufptr[idx];
	uint8 *buffer = &net_input_buffer[idx][0];

	// command info
	uint16 input_len;
	uint16 command;
	uint32 command_handle;
	uint8 *command_data;

	while(len >= 8){ // commands are at least 8 bytes
		input_len = decode_u16_le(buffer + 0);
		DEBUG_ASSERT(input_len <= len && input_len >= 8);
		command = decode_u16_le(buffer + 2);
		command_handle = decode_u32_le(buffer + 4);
		command_data = buffer + 8;

		if(command == CMD_ACCOUNT_LOGIN){
			uint32 xtea[4];
			char accname[32];
			char password[32];
			uint16 A; // accumulator

			xtea[0] = decode_u32_le(command_data + 0);
			xtea[1] = decode_u32_le(command_data + 4);
			xtea[2] = decode_u32_le(command_data + 8);
			xtea[3] = decode_u32_le(command_data + 12);

			A = decode_tibia_string(command_data + 16, accname, 32);
			A += decode_tibia_string(command_data + 16 + A, password, 32);

			LOG("xtea = {%08X, %08X, %08X, %08X}",
				decode_u32_le(command_data + 0),
				decode_u32_le(command_data + 4),
				decode_u32_le(command_data + 8),
				decode_u32_le(command_data + 12));
			LOG("accname = '%s', password = '%s'", accname, password);
		}

		DEBUG_LOG("net input: len = %u, command = %u, command_handle = %u",
			(unsigned)input_len, (unsigned)command, (unsigned)command_handle);
		buffer += input_len;
		len -= input_len;
	}
}

bool game_init(void){
	net_input_bufptr[0] = 0;
	net_input_bufptr[1] = 0;
	return true;
}

void game_shutdown(void){
}

static void game_server_sync_routine(void *arg){
	// 1 - iterate over LOGIN RESOLVES and send it
	/*
	for(resolve in login_resolves){
		protocol_login_send(resolve.connection, resolve.output);
	}
	*/

	// 2 - iterate over PLAYERS and if they have an outbuf, send it
	/* METHOD 1
	{ // OUTSIDE THE SYNC ROUTINE
		struct outbuf *output;
		struct{
			uint32 connection;
			struct outbuf *output;
		} output_list[MAX_PLAYERS];
		int num_output = 0;
		for(player in players){
			output = player.output;
			if(output != NULL){
				output_list[num_output].connection = player.connection;
				output_list[num_output].output = output;
				num_output += 1;
			}
		}
	}

	{ // NOW INSIDE THE SYNC ROUTINE
		for(int i = 0; i < num_output; i += 1){
			connection_send(output_list[i].connection,
				output_list[i].output->data,
				output_list[i].output->datalen);
		}
	}
	*/
	/* METHOD 2
	for(player in players){
		protocol_game_send(player);
	}

	*/

	// 3 - swap the input buffers
	net_idx = 1 - net_idx;
	net_input_bufptr[net_idx] = 0;
}

// frame interval in milliseconds:
//	16 is ~60fps
//	33 is ~30fps
//	66 is ~15fps
#define GAME_FRAME_INTERVAL 33

void game_run(void){
	int64 frame_start;
	int64 frame_end;
	int64 next_frame;
	while(1){
		// calculate frame times so each frame takes the
		// same time to complete (in a perfect scenario)
		frame_start = kpl_clock_monotonic_msec();
		next_frame = frame_start + GAME_FRAME_INTERVAL;

		// do work
		server_sync(game_server_sync_routine, NULL);
		game_consume_net_input();

		// stall until the next frame if we finished too early
		frame_end = kpl_clock_monotonic_msec();
		if(frame_end < next_frame)
			kpl_sleep_msec(next_frame - frame_end);
		DEBUG_LOG("game frame running time: %lld", frame_end - frame_start);
		DEBUG_LOG("game frame idle time: %lld", next_frame - frame_end);
	}
}

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
