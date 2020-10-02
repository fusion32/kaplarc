/*
 * This protocol will handle login requests from clients 8.30 - 8.60 which
 * add a checksum to its messages. For clients prior to 8.30 there will be
 * a separate protocol that will account for the lack of it.
 */

/*
 *  The tibia protocol includes two types of messages. The first is the login
 * message which is RSA ENCODED and contains login information and a XTEA KEY
 * that is used for further communication.
 *  The second type is the common message which uses the key received in the
 * login message and is XTEA ENCODED.
 *  Both types have the same general structure but their bodies will differ.
 * Also, LENGTH parameters don't include their own size and values with more
 * than one byte are encoded in little endian.
 *
 * GENERAL MESSAGE STRUCTURE
 *	00	LENGTH
 *	02	BODY
 *
 * ACCOUNT LOGIN BODY STRUCTURE
 *	00	ADLER32 CHECKSUM
 *	04	PROTOCOL ID
 *	05	CLIENT OS
 *	07	CLIENT VERSION
 *	09	CLIENT .DAT .SPR .PIC SIGNATURES
 *	21	RSA ENCODED DATA
 *
 *	RSA ENCODED DATA (after DECODING)
 *		00	XTEA KEY
 *		16	ACCOUNT NAME	(A bytes where A < 32, S = A)
 *		16 + S	PASSWORD	(B bytes where B < 32, S += B)
 *
 * PLAYER LOGIN BODY STRUCTURE
 *	00	ADLER32 CHECKSUM
 *	04	LEGACY PROTOCOL ID
 *	05	CLIENT OS
 *	07	CLIENT VERSION
 *	09	RSA ENCODED DATA
 *
 *	RSA ENCODED DATA (after DECODING)
 *		00	XTEA KEY
 *		16	GAMEMASTER FLAG (?)
 *		17	ACCOUNT NAME	(A bytes where A < 32, S = A)
 *		17 + S	CHARACTER NAME	(B bytes where B < 32, S += B)
 *		17 + S	PASSWORD	(C bytes where C < 32, S += C)
 *		17 + S	CHALLENGE REPLY (?)
 *
 * COMMON BODY STRUCTURE
 *	00	ADLER32 CHECKSUM
 *	04	XTEA ENCODED DATA
 *
 *	XTEA ENCODED DATA (after DECODING)
 *		00	DECODED LENGTH
 *		02	DECODED DATA
 *
 */

#include "buffer_util.h"
#include "config.h"
#include "game.h"
#include "outbuf.h"
#include "tibia_rsa.h"
#include "server/server.h"
#include "crypto/xtea.h"
#include "db/database.h"

/* LIFETIME OF `login_info`
 *	Any of the functions that take `struct login_info` as a parameter
 *	WILL take ownership of it. The structure lifetime starts at
 *	`on_recv_first_message` and ends at `internal_resolve_login`
 *	regardless. The path may change but this should always be the case.
 */

struct login_info{
	struct outbuf *output;
	uint32 connection;
	uint32 xtea[4];
	char accname[32];
	char password[32];
};

static void outbuf_prepare(struct outbuf *buf){
	// reserve 8 bytes for header
	buf->ptr = buf->base + 8;
}

static void outbuf_wrap(struct outbuf *buf, uint32 *xtea){
	uint8 *data = buf->base + 6;
	uint32 datalen = (uint32)(buf->ptr - data);
	uint32 checksum;
	int padding = (8 - (datalen & 7)) & 7;

	// xtea encode
	encode_u16_le(data, datalen-2);
	datalen += padding;
	while(padding-- > 0)
		outbuf_write_byte(buf, 0x33);
	xtea_encode(xtea, data, datalen);

	// add message header
	checksum = adler32(data, datalen);
	encode_u16_le(buf->base, datalen+4);
	encode_u32_le(buf->base+2, checksum);
}

static void internal_resolve_login(void *arg){
	DEBUG_ASSERT(arg != NULL); // PARANOID
	struct login_info *login = arg;
	void **udata = connection_userdata(login->connection);
	if(login->output != NULL){
		if(udata != NULL){
			*udata = login->output;
			connection_send(login->connection,
				outbuf_data(login->output),
				outbuf_len(login->output));
		}else{
			// if the connection is no loger valid we
			// need to release the outbuf HERE
			outbuf_release(login->output);
		}
	}
	// clear password to be extra safe
	memset(login->password, 0, sizeof(login->password));
	// always release the login data here
	kpl_free(login);
}

static void build_disconnect_message(struct login_info *login, const char *message){
	DEBUG_ASSERT(login->output == NULL); // PARANOID
	struct outbuf *buf = login->output = outbuf_acquire();
	outbuf_prepare(buf);
	outbuf_write_byte(buf, 0x0A);
	outbuf_write_str(buf, message);
	outbuf_wrap(buf, login->xtea);
}

static void internal_send_disconnect(struct login_info *login, const char *message){
	build_disconnect_message(login, message);
	internal_resolve_login(login);
}

static void send_disconnect(struct login_info *login, const char *message){
	build_disconnect_message(login, message);
	game_add_server_task(internal_resolve_login, login);
}

static void database_resolve_login(void *udata){
	struct login_info *login = udata;
	struct outbuf *buf;
	db_result_t *res;
	int32 accid;
	int64 premend;
	const char *pwd;
	int nrows;

	if(login->accname[0] == 0){
		send_disconnect(login, "Invalid account name.");
		return;
	}

	// load account info
	res = db_load_account_info(login->accname);
	nrows = db_result_nrows(res);
	if(res == NULL || nrows == 0){
		if(nrows == 0)
			db_result_clear(res);
		send_disconnect(login, "Account name or password is not correct.");
		return;
	}
	DEBUG_ASSERT(nrows == 1); // PARANOID
	accid = db_result_get_int32(res, 0, DBRES_ACC_INFO_ID);
	premend = db_result_get_int64(res, 0, DBRES_ACC_INFO_PREMEND);
	pwd = db_result_get_value(res, 0, DBRES_ACC_INFO_PASSWORD);
	if(strcmp(pwd, login->password) != 0){ //@TODO: use bcrypt or some other hashing
		db_result_clear(res);
		send_disconnect(login, "Account name or password is not correct.");
		return;
	}
	db_result_clear(res);

	// @TODO: CHECK IF ACC BANNED
	// @TODO: CHECK IF IP BANNED

	// load charlist
	res = db_load_account_charlist(accid);
	if(res == NULL){
		send_disconnect(login, "Internal error. Contact an admin.");
		return;
	}
	// send charlist message
	nrows = db_result_nrows(res);
	buf = login->output = outbuf_acquire();
	outbuf_prepare(buf);
	// motd
	outbuf_write_byte(buf, 0x14);
	outbuf_write_str(buf, config_get("motd"));
	// charlist
	outbuf_write_byte(buf, 0x64);
	outbuf_write_byte(buf, nrows);
	for(int i = 0; i < nrows; i += 1){
		outbuf_write_str(buf,
			db_result_get_value(res, i, DBRES_ACC_CHARLIST_NAME));
		outbuf_write_str(buf, config_get("sv_name"));
		outbuf_write_u32(buf, 16777343); // (localhost) @TODO: resolve addr from config sv_addr
		outbuf_write_u16(buf, (uint16)config_geti("sv_game_port"));
	}
	outbuf_write_u16(buf, 1); // @TODO: calc premdays from premend = days_until(premend)
	outbuf_wrap(buf, login->xtea);
	db_result_clear(res);
	game_add_server_task(internal_resolve_login, login);
}

/* PROTOCOL IMPL */
static bool identify(uint8 *data, uint32 datalen){
	uint32 checksum;
	if(datalen < 5)
		return false;
	checksum = adler32(data+4, datalen-4);
	return checksum == decode_u32_le(data)
		&& data[4] == 0x01;
}

static bool on_assign_protocol(uint32 c){
	*connection_userdata(c) = NULL;
	return true;
}

static void on_close(uint32 c){
	DEBUG_LOG("protocol_login: on_close");

	// make sure the outbuf is released in case of a write error
	void **udata = connection_userdata(c);
	struct outbuf *buf = *udata;
	if(buf != NULL){
		outbuf_release(buf);
		*udata = NULL;
	}
}

static protocol_status_t on_write(uint32 c){
	void **udata = connection_userdata(c);
	struct outbuf *buf = *udata;
	DEBUG_ASSERT(buf != NULL);
	outbuf_release(buf);
	*udata = NULL;
	return PROTO_CLOSE;
}

static protocol_status_t on_recv_message(uint32 c, uint8 *data, uint32 datalen){
	return PROTO_ABORT; // should no happen
}

static protocol_status_t on_recv_first_message(uint32 c, uint8 *data, uint32 datalen){
	struct login_info *login;
	uint8 *decoded;
	size_t decoded_len;
	uint16 version, A, B;

	if(datalen != 149){
		DEBUG_LOG("protocol_login: invalid login message length"
			" (expected = %d, got = %d)", 149, datalen);
		return PROTO_CLOSE;
	}

	version = decode_u16_le(data + 7);
	// version <= 760 didn't have RSA + XTEA encryption
	// version <  830 didn't have checksum
	// shouldn't happen unless it's a purposely malformed message
	if(version < 830)
		return PROTO_CLOSE;

	// rsa decode
	decoded = data + 21;
	if(!tibia_rsa_decode(decoded, 128, &decoded_len))
		return PROTO_CLOSE;

	// `decoded_len` seems to be constant and the extra bytes are
	// just padding bytes
	if(decoded_len != 127)
		return PROTO_CLOSE;

	login = kpl_malloc(sizeof(struct login_info));
	login->connection = c;
	login->output = NULL;
	login->xtea[0] = decode_u32_le(decoded + 0);
	login->xtea[1] = decode_u32_le(decoded + 4);
	login->xtea[2] = decode_u32_le(decoded + 8);
	login->xtea[3] = decode_u32_le(decoded + 12);

	if(version < TIBIA_CLIENT_VERSION_MIN || version > TIBIA_CLIENT_VERSION_MAX){
		internal_send_disconnect(login, "This server requires client"
			" version " TIBIA_CLIENT_VERSION_STR ".");
		return PROTO_CLOSE;
	}

	// the client doesn't allow for account name or password to be
	// larger than 30 bytes
	A = decode_tibia_string(decoded + 16, login->accname, sizeof(login->accname));
	B = (A > 32) ? 0
		: decode_tibia_string(decoded + 16 + A,
			login->password, sizeof(login->password));
	if(A > 32 || B > 32){
		internal_send_disconnect(login, "Your account has been banned.");
		return PROTO_CLOSE;
	}

	DEBUG_LOG("account_login");
	DEBUG_LOG("xtea = {%08X, %08X, %08X, %08X}",
		login->xtea[0], login->xtea[1],
		login->xtea[2], login->xtea[3]);
	LOG("accname = '%s', password = '%s'",
		login->accname, login->password);

	if(!db_add_task(database_resolve_login, login)){
		internal_send_disconnect(login, "Internal error. Try again later.");
		return PROTO_CLOSE;
	}
	return PROTO_STOP_READING;
}

/* PROTOCOL DECL */
struct protocol protocol_login = {
	.name = "login",
	.sends_first = false,
	.identify = identify,

	.on_assign_protocol = on_assign_protocol,
	.on_close = on_close,
	.on_connect = NULL,
	.on_write = on_write,
	.on_recv_message = on_recv_message,
	.on_recv_first_message = on_recv_first_message,
};
