#include "buffer_util.h"
#include "config.h"
#include "outbuf.h"
#include "tibia_rsa.h"
#include "server/server.h"
#include "crypto/xtea.h"
#include "db/database.h"

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool on_assign_protocol(uint32 c);
static void on_close(uint32 c);
static protocol_status_t on_write(uint32 c);
static protocol_status_t on_recv_message(uint32 c, uint8 *data, uint32 datalen);
static protocol_status_t on_recv_first_message(uint32 c, uint8 *data, uint32 datalen);
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

/* IMPL START */
static bool identify(uint8 *data, uint32 datalen){
	// @NOTE: messages without checksum will use
	// the old login protocol
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

struct login_data{
	uint32 connection;
	uint32 xtea[4];
	char accname[32];
	char password[32];
};

//static void internal_outbuf
static void internal_send_disconnect(struct login_data *login, const char *message);
static void resolve_login(void *lstore, size_t lstore_size);

static protocol_status_t on_recv_first_message(uint32 c, uint8 *data, uint32 datalen){
	struct login_data login = { .connection = c };
	uint8 *decoded;
	size_t decoded_len;
	uint16 version, A;

	// data + 0 == checksum (4 bytes)	|-> verified at the `identify` function
	// data + 4 == protocol id (1 bytes)	|
	// data + 5 == client os (2 bytes)
	// data + 7 == client version (2 bytes)
	// data + 9 == tibia .dat, .spr, .pic checksum (12 bytes)
	// data + 21 == rsa encoded data (128 bytes)
	if(datalen != 149){
		DEBUG_LOG("protocol_login: invalid login message length"
			" (expected = %d, got = %d)", 149, datalen);
		return PROTO_CLOSE;
	}

	version = decode_u16_le(data + 7);
	// version <= 760 didn't have RSA + XTEA encryption
	// version <= 822 didn't have checksum
	// shouldn't happen unless it's a purposely malformed message
	if(version <= 822)
		return PROTO_CLOSE;

	// rsa decode
	decoded = data + 21;
	if(!tibia_rsa_decode(decoded, 128, &decoded_len))
		return PROTO_CLOSE;

	// decoded + 0 == xtea key (16 bytes)
	// decoded + 16 == accname length (2 bytes)
	// decoded + 18 == accname data (<= 30 bytes)
	// decoded + 18 + X == password length (2 bytes)
	// decoded + 20 + X == password data (<= 30 bytes)
	// decoded + 16 == accname (A bytes)
	// decoded + 16 + A == password (B bytes)
	if(decoded_len != 127)
		// this value seems to be constant and the bytes after
		// the password are just padding bytes
		return PROTO_CLOSE;

	login.xtea[0] = decode_u32_le(decoded + 0);
	login.xtea[1] = decode_u32_le(decoded + 4);
	login.xtea[2] = decode_u32_le(decoded + 8);
	login.xtea[3] = decode_u32_le(decoded + 12);

	if(version < TIBIA_CLIENT_VERSION_MIN || version > TIBIA_CLIENT_VERSION_MAX){
		// send_disconnect
		return PROTO_CLOSE;
	}

	// the client doesn't allow for accname or password to be
	// larger than 30 bytes so we don't need to send a disconnect
	// message if the login message is bad
	A = decode_tibia_string(decoded + 16, login.accname, sizeof(login.accname));
	if(A > 32) return PROTO_CLOSE;
	A = decode_tibia_string(decoded + 16 + A, login.password, sizeof(login.password));
	if(A > 32) return PROTO_CLOSE;

	// send login request to the database thread and stop reading
	db_add_task(resolve_login, &login, sizeof(login));
	return PROTO_STOP_READING;
}

static void resolve_login(void *lstore, size_t lstore_size){
	//@TODO: i forgot i can't do a connection_send from outside the
	// net thread so we need to send a request to the game thread
	// or add some synchronization for parallel connection_sends

	DEBUG_ASSERT(lstore_size == sizeof(struct login_data));
	struct login_data *login = lstore;
	struct outbuf *buf;
	db_result_t *res;
	int32 accid;
	int64 premend;
	const char *pwd;
	int nrows;


	if(login->accname[0] == 0){
		// "Invalid account name."
		return;
	}

	// validate login
	res = db_load_account_info(login->accname);
	if(res == NULL){
		// "Account name or password is not correct."
		return;
	}
	DEBUG_ASSERT(db_result_nrows(res) == 1); // PARANOID
	accid = db_result_get_int32(res, 0, DB_RESULT_ACCOUNT_INFO_ID);
	premend = db_result_get_int64(res, 0, DB_RESULT_ACCOUNT_INFO_PREMEND);
	pwd = db_result_get_value(res, 0, DB_RESULT_ACCOUNT_INFO_PASSWORD);
	//if(!(check_password)){
		// "Account name or password is not correct."
		// db_result_clear(res);
	//}
	db_result_clear(res);

	// @TODO: CHECK IF ACC BANNED
	// @TODO: CHECK IF IP BANNED

	// load charlist
	res = db_load_account_charlist(accid);
	if(res == NULL){
		// "Internal error."
		return;
	}

	// send charlist message
	nrows = db_result_nrows(res);
	buf = outbuf_acquire();
	internal_outbuf_prepare(buf);
	// motd
	outbuf_write_byte(buf, 0x14);
	outbuf_write_str(&buf, config_get("motd"));
	// charlist
	outbuf_write_byte(buf, 0x64);
	outbuf_write_byte(buf, nrows);
	for(int i = 0; i < nrows; i += 1){
		outbuf_write_str(buf,
			db_result_get_value(res, i, DBRES_CHARLIST_NAME));
		outbuf_write_str(buf, config_get("sv_name"));
		outbuf_write_u32(buf, 1677343); // localhost @TODO: resolve addr from config sv_addr
		outbuf_write_u16(buf, (uint16)config_geti("sv_game_port"));
	}
	outbuf_write_u16(buf, 1); // @TODO: calc premdays from premend
	// wrap outbuf
	internal_outbuf_wrap(buf, login->xtea);

	// @TODO SEND
	outbuf_release(buf);
}

static void internal_outbuf_prepare(struct outbuf *buf){
	// reserve 8 bytes for the header
	//  2 bytes - message length
	//  4 bytes - checksum
	//  2 bytes - unencrypted length
	buf->ptr = buf->base + 8;
}

static void internal_outbuf_wrap(struct outbuf *buf, uint32 *xtea){
	// NOTE1: both the message length and unencrypted
	// length don't include their own size
	// NOTE2: the unencrypted length is encrypted
	// together with the data
	uint8 *data = buf->base + 6;
	uint32 datalen = buf->ptr - buf->base;
	uint32 checksum;
	int padding = (8 - (datalen & 7)) & 7;

	// set unencrypted length (datalen - 2)
	encode_u16_le(data, datalen-2);
	// add padding for xtea encoding
	datalen += padding;
	while(padding-- > 0)
		outbuf_write_byte(buf, 0x33);
	// xtea encode
	xtea_encode(xtea, data, datalen);

	// calc checksum
	checksum = adler32(data, datalen);
	// set message length (datalen + 6 - 2)
	encode_u16_le(buf->base, datalen+4);
	// set checksum
	encode_u32_le(buf->base+2, checksum);
}

static void internal_outbuf_write_disconnect(struct outbuf *buf, const char *message){
	// prepare outbuf
	internal_outbuf_prepare(buf);

	// add disconnect message
	outbuf_write_byte(buf, 0x0A);
	outbuf_write_str(buf, message);
}

void protocol_login_outbuf_charlist_write_character(struct outbuf *buf, const char *name){
	outbuf_write_str(buf, name);
	outbuf_write_str(buf, config_get("sv_name"));
	//outbuf_write_u32(buf, config_get());
	outbuf_write_u32(buf, 16777343); // localhost
	outbuf_write_u16(buf, (uint16)config_geti("sv_game_port"));
}

void protocol_login_outbuf_charlist_end(struct outbuf *buf, uint16 premdays){
	// charlist end
	outbuf_write_u16(buf, premdays);
}

bool protocol_login_send(uint32 connection, uint32 *xtea, struct outbuf *buf){
	void **udata = connection_userdata(connection);
	if(udata == NULL) // connection already dead
		return;
	internal_outbuf_wrap(buf, xtea);
	connection_send(connection, buf->base, buf->ptr - buf->base);

	// this is necessary so we are able to release the
	// outbuf inside on_write after the write is complete
	// (note that there should only ever be one send operation
	// on the login protocol and we assume that in here)
	DEBUG_ASSERT(*udata == NULL);
	*udata = buf;
}
