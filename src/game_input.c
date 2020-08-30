#include "game.h"
#include "buffer_util.h"


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

static void cmd_account_login(uint32 connection, uint8 *data){
	uint32 xtea[4];
	char accname[32];
	char password[32];
	uint16 A;

	xtea[0] = decode_u32_le(data + 0);
	xtea[1] = decode_u32_le(data + 4);
	xtea[2] = decode_u32_le(data + 8);
	xtea[3] = decode_u32_le(data + 12);

	A = decode_tibia_string(data + 16, accname, 32);
	A += decode_tibia_string(data + 16 + A, password, 32);

	LOG("cmd_account_login:");
	LOG("xtea = {%08X, %08X, %08X, %08X}",
		xtea[0], xtea[1], xtea[2], xtea[3]);
	LOG("accname = '%s', password = '%s'", accname, password);
}

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

		switch(command){
		case CMD_ACCOUNT_LOGIN:
			cmd_account_login(command_handle, command_data);
			break;
		case CMD_PLAYER_LOGIN:
		case CMD_PLAYER_LOGOUT:
		case CMD_PLAYER_KEEP_ALIVE:
		case CMD_PLAYER_AUTO_WALK:
		case CMD_PLAYER_MOVE_NORTH:
		case CMD_PLAYER_MOVE_EAST:
		case CMD_PLAYER_MOVE_SOUTH:
		case CMD_PLAYER_MOVE_WEST:
		case CMD_PLAYER_STOP_AUTO_WALK:
		case CMD_PLAYER_MOVE_NORTHEAST:
		case CMD_PLAYER_MOVE_SOUTHEAST:
		case CMD_PLAYER_MOVE_SOUTHWEST:
		case CMD_PLAYER_MOVE_NORTHWEST:
		case CMD_PLAYER_TURN_NORTH:
		case CMD_PLAYER_TURN_EAST:
		case CMD_PLAYER_TURN_SOUTH:
		case CMD_PLAYER_TURN_WEST:
		case CMD_PLAYER_ITEM_THROW:
		case CMD_PLAYER_SHOP_LOOK:
		case CMD_PLAYER_SHOP_BUY:
		case CMD_PLAYER_SHOP_SELL:
		case CMD_PLAYER_SHOP_CLOSE:
		case CMD_PLAYER_TRADE_REQUEST:
		case CMD_PLAYER_TRADE_LOOK:
		case CMD_PLAYER_TRADE_ACCEPT:
		case CMD_PLAYER_TRADE_CLOSE:
		case CMD_PLAYER_ITEM_USE:
		case CMD_PLAYER_ITEM_USE_EX:
		case CMD_PLAYER_BATTLE_WINDOW:
		case CMD_PLAYER_ITEM_ROTATE:
		case CMD_PLAYER_CONTAINER_CLOSE:
		case CMD_PLAYER_CONTAINER_PARENT:
		case CMD_PLAYER_ITEM_WRITE:
		case CMD_PLAYER_HOUSE_WRITE:
		case CMD_PLAYER_LOOK:
		case CMD_PLAYER_SAY:
		case CMD_PLAYER_CHANNELS_REQUEST:
		case CMD_PLAYER_CHANNEL_OPEN:
		case CMD_PLAYER_CHANNEL_CLOSE:
		case CMD_PLAYER_PVT_CHAT_OPEN:
		case CMD_PLAYER_NPC_CHAT_CLOSE:
		case CMD_PLAYER_BATTLE_MODE:
		case CMD_PLAYER_ATTACK:
		case CMD_PLAYER_FOLLOW:
		case CMD_PLAYER_PARTY_INVITE:
		case CMD_PLAYER_PARTY_JOIN:
		case CMD_PLAYER_PARTY_INVITE_REVOKE:
		case CMD_PLAYER_PARTY_LEADERSHIP_PASS:
		case CMD_PLAYER_PARTY_LEAVE:
		case CMD_PLAYER_PARTY_SHARED_EXP:
		case CMD_PLAYER_CHANNEL_CREATE:
		case CMD_PLAYER_CHANNEL_INVITE:
		case CMD_PLAYER_CHANNEL_KICK:
		case CMD_PLAYER_HOLD:
		case CMD_PLAYER_TILE_UPDATE_REQUEST:
		case CMD_PLAYER_CONTAINER_UPDATE_REQUEST:
		case CMD_PLAYER_OUTFITS_REQUEST:
		case CMD_PLAYER_OUTFIT_SET:
		//case CMD_PLAYER_MOUNT:
		case CMD_PLAYER_VIP_ADD:
		case CMD_PLAYER_VIP_REMOVE:
		//case CMD_PLAYER_BUG_REPORT:
		//case CMD_PLAYER_VIOLATION_WINDOW:
		//case CMD_PLAYER_DEBUG_ASSERT:
		case CMD_PLAYER_QUESTLOG_REQUEST:
		case CMD_PLAYER_QUESTLINE_REQUEST:
		//case PLAYER_VIOLATION_REPORT:
		default:
			break;
		}

		DEBUG_LOG("net input: len = %u, command = %u, command_handle = %u",
			(unsigned)input_len, (unsigned)command, (unsigned)command_handle);
		buffer += input_len;
		len -= input_len;
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

