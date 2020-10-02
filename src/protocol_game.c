/*

class ProtocolGame : public Protocol
{
private:
  std::list<uint32_t> knownCreatureList;
  Player* player;
  uint32_t eventConnect;
  bool m_debugAssertSent;
  bool m_acceptPackets;
};

#define KNOWN_MAX_CREATURES 150
struct player_handle{
	struct outbuf *output_queue;
	uint32 connection;
	uint32 xtea[4];
	union{
		struct{
			char accname[32];
			char password[32];
			char charname[32];
		} login_info;
		struct{
			uint32 player;
			uint32 num_known_creatures;
			uint32 known_creatures[KNOWN_MAX_CREATURES];
		} player_info;
	};
};

*/

#include "buffer_util.h"
#include "common.h"
#include "tibia_rsa.h"
#include "server/protocol.h"
#include "server/server.h"


struct login_info{
	struct outbuf *output_queue;
	uint32 connection;
	uint32 xtea[4];
	bool gm_flag;
	char accname[32];
	char password[32];
	char charname[32];
};

static void database_resolve_login(void *udata){

}

/* PROTOCOL IMPL */
static bool on_assign_protocol(uint32 c){
	*connection_userdata(c) = NULL;
	return true;
}

static void on_close(uint32 c){
	DEBUG_LOG("game on close");
}

static protocol_status_t on_connect(uint32 c){
	DEBUG_LOG("game on connect");

	// this seems to be some kind of challenge message that
	// is simply copied and attached at the login message
	// after the accname, charname, and password
	static const uint8 data[] = {
		0x0C, 0x00,				// message total length
		0x23, 0x03, 0xE8, 0x0A,			// message checksum
		0x06, 0x00,				// message data length
		0x1F, 0xFF, 0xFF, 0x00, 0x00, 0xFF	// message data
	};
	connection_send(c, (uint8*)data, sizeof(data));
	return PROTO_OK;
}

static protocol_status_t on_write(uint32 c){
	return PROTO_OK;
}
static protocol_status_t on_recv_message(uint32 c, uint8 *data, uint32 datalen){
	return PROTO_CLOSE;
}
static protocol_status_t on_recv_first_message(uint32 c, uint8 *data, uint32 datalen){
	// @NOTE: See `protocol_login.on_recv_first_message`
	// comments if something is unclear. They're almost
	// the same function so I omitted common comments in here.
	struct login_info *login;
	uint8 *decoded;
	size_t decoded_len;
	uint16 version, A, B, C, S;

	if(datalen != 137){
		DEBUG_LOG("protocol_game: invalid login message length"
			" (expected = %d, got = %d)", 137, datalen);
		return PROTO_CLOSE;
	}

	version = decode_u16_le(data + 7);
	if(version < 830)
		return PROTO_CLOSE;

	// rsa decode
	decoded = data + 9;
	if(!tibia_rsa_decode(decoded, 128, &decoded_len))
		return PROTO_CLOSE;
	if(decoded_len != 127)
		return PROTO_CLOSE;

	login = kpl_malloc(sizeof(struct login_info));
	login->xtea[0] = decode_u32_le(decoded + 0);
	login->xtea[1] = decode_u32_le(decoded + 4);
	login->xtea[2] = decode_u32_le(decoded + 8);
	login->xtea[3] = decode_u32_le(decoded + 12);

	if(version < TIBIA_CLIENT_VERSION_MIN || version > TIBIA_CLIENT_VERSION_MAX){
		kpl_free(login); // "This server requires client version " TIBIA_CLIENT_VERSION_STR "."
		return PROTO_CLOSE;
	}

	// (?)
	login->gm_flag = decode_u8(decoded + 16) == 0x01;

	// the client doesn't allow for account name, password or character
	// name to be larger than 30 bytes
	S = A = decode_tibia_string(decoded + 17,
			login->accname, sizeof(login->accname));
	S += B = (A > 32) ? 0
		: decode_tibia_string(decoded + 17 + S,
			login->charname, sizeof(login->charname));
	S += C = (B > 32) ? 0
		: decode_tibia_string(decoded + 17 + S,
			login->password, sizeof(login->password));
	if(A > 32 || B > 32 || C > 32){
		kpl_free(login); //"Your account has been banned."
		return PROTO_CLOSE;
	}

	// see `on_connect` about this data
	// decode_u32_le(decoded + 17 + S) == 0x0000FFFF;
	// decode_u8(decoded + 21 + S) == 0xFF;

	DEBUG_LOG("player login:");
	DEBUG_LOG("xtea = {%08X, %08X, %08X, %08X}",
		login->xtea[0], login->xtea[1],
		login->xtea[2], login->xtea[3]);
	LOG("accname = '%s', password = '%s'",
		login->accname, login->password);
	LOG("charname = '%s'", login->charname);

	//if(!db_add_task(database_resolve_login, login)){
	//	kpl_free(login); //"Internal error. Try again later."
	//	return PROTO_CLOSE;
	//}
	kpl_free(login);
	return PROTO_CLOSE;
	//return PROTO_OK;
}

/* PROTOCOL DECL */
struct protocol protocol_game = {
	.name = "game",
	.sends_first = true,
	.identify = NULL,

	.on_assign_protocol = on_assign_protocol,
	.on_close = on_close,
	.on_connect = on_connect,
	.on_write = on_write,
	.on_recv_message = on_recv_message,
	.on_recv_first_message = on_recv_first_message,
};

