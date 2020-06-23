#include "buffer_util.h"
#include "config.h"
#include "game.h"
#include "hash.h"
#include "packed_data.h"
#include "tibia_rsa.h"
#include "server/server.h"
#include "crypto/xtea.h"

/* LOGIN OUTPUT BUFFER */
#define LOGIN_HANDLE_SIZE sizeof(login_handle_t)
#define LOGIN_BUFFER_SIZE 1024
typedef union{
	struct{
		uint32 connection;
		uint32 xtea[4];
	};
	uint8 output_buffer[LOGIN_BUFFER_SIZE];
} login_handle_t;


struct login_protocol_handle{
	uint32 connection;
	uint32 output_length;
	void *output_buffer;
};

struct game_protocol_handle{
	uint32 connection;
	uint32 output_lengths[2];
	void *output_buffers[2];
};

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool create_handle(uint32 connection, void **handle);
static void destroy_handle(uint32 connection, void *handle);
static void on_close(uint32 connection, void *handle);
static protocol_status_t on_connect(uint32 connection, void *handle);
static protocol_status_t on_write(uint32 connection, void *handle);
static protocol_status_t on_recv_message(uint32 connection,
	void *handle, uint8 *data, uint32 datalen);
static protocol_status_t on_recv_first_message(uint32 connection,
	void *handle, uint8 *data, uint32 datalen);
struct protocol protocol_login = {
	.name = "login",
	.sends_first = false,
	.identify = identify,

	.create_handle = create_handle,
	.destroy_handle = destroy_handle,
	.on_close = on_close,
	.on_connect = on_connect,
	.on_write = on_write,
	.on_recv_message = on_recv_message,
	.on_recv_first_message = on_recv_first_message,
};

/* IMPL START */
bool identify(uint8 *data, uint32 datalen){
	// @NOTE: messages without checksum will use
	// the old login protocol
	uint32 checksum;
	if(datalen < 5)
		return false;
	checksum = adler32(data+4, datalen-4);
	return checksum == decode_u32_le(data)
		&& data[4] == 0x01;
}

static bool create_handle(uint32 connection, void **handle){
	// no op
	return true;
}

static void destroy_handle(uint32 connection, void *handle){
	// no op
}

static void on_close(uint32 connection, void *handle){
	// no op
	DEBUG_LOG("protocol_login: on_close");
}

static protocol_status_t on_connect(uint32 connection, void *handle){
	// no op
	return PROTO_OK;
}

static protocol_status_t on_write(uint32 connection, void *handle){
	// close protocol after the disconnect message or
	// character list has been sent
	return PROTO_CLOSE;
}

static protocol_status_t on_recv_message(uint32 connection,
		void *handle, uint8 *data, uint32 datalen){
	// no op
	return PROTO_OK;
}

static protocol_status_t on_recv_first_message(uint32 connection,
		void *handle, uint8 *data, uint32 datalen){
	uint8 *decoded;
	size_t decoded_len;
	uint16 accname_len;
	uint16 password_len;
	uint16 version;

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
	// version <= 760 didn't have checksum so this shouldn't
	// happen unless its a purposely malformed message
	if(version <= 760)
		return PROTO_CLOSE;

	// rsa decode
	decoded = data + 21;
	if(!tibia_rsa_decode(decoded, 128, &decoded_len))
		return PROTO_CLOSE;

	// decoded + 0 == xtea key (16 bytes)
	// decoded + 16 == accname length (2 bytes)
	// decoded + 18 == accname data (<= 30 bytes)
	// decoded + 18 + V == password length (2 bytes)
	// decoded + 20 + V == password data (<= 30 bytes)
	if(decoded_len != 127)
		// this value seems to be constant and the bytes after
		// the password are just padding bytes
		return PROTO_CLOSE;

	// @NOTE: the client doesn't allow for accname or password
	// to be larger than 30 bytes so we don't need to send a
	// disconnect message if the login message is bad

	// check if accname is valid
	accname_len = decode_u16_le(decoded + 16);
	if(accname_len > 30)
		return PROTO_CLOSE;

	// check if password is valid
	password_len = decode_u16_le(decoded + 18 + accname_len);
	if(password_len > 30)
		return PROTO_CLOSE;

	DEBUG_LOG("xtea = {%08X, %08X, %08X, %08X}",
		decode_u32_le(decoded + 0), decode_u32_le(decoded + 4),
		decode_u32_le(decoded + 8), decode_u32_le(decoded + 12));
	DEBUG_LOG("account name = `%.*s`", accname_len, (decoded + 18));
	DEBUG_LOG("account password = `%.*s`",
		password_len, (decoded + 20 + accname_len));


	// send login request to the game thread and stop reading
	// (in the worst case it is timed out)
	game_add_net_input(CMD_ACCOUNT_LOGIN, connection,
		decoded, (20 + accname_len + password_len));
	return PROTO_STOP_READING;
}

static void writer_begin(struct data_writer *writer){
	DEBUG_ASSERT(writer != NULL);
	// skip 8 bytes:
	//  2 bytes - message length
	//  4 bytes - checksum
	//  2 bytes - unencrypted length
	writer->ptr = writer->base + 8;
}

static void writer_end(struct data_writer *writer, uint32 *xtea){
	DEBUG_ASSERT(writer != NULL);
	DEBUG_ASSERT(xtea != NULL);

	// NOTE1: both the message length and unencrypted
	// length don't include their own size
	// NOTE2: the unencrypted length is encrypted
	// together with the data
	uint8 *data = writer->base + 6;
	uint32 datalen = (uint32)(writer->ptr - data);
	uint32 checksum;
	int padding = (8 - (datalen & 7)) & 7;

	// set unencrypted length (datalen - 2)
	encode_u16_le(data, datalen-2);
	// add padding for xtea encoding
	datalen += padding;
	while(padding-- > 0)
		data_write_byte(writer, 0x33);
	// xtea encode
	xtea_encode(xtea, data, datalen);

	// calc checksum
	checksum = adler32(data, datalen);
	// set message length (datalen + 6 - 2)
	encode_u16_le(writer->base, datalen+4);
	// set checksum
	encode_u32_le(writer->base+2, checksum);
}

#if 0
static void send_disconnect(uint32 connection, uint32 *xtea, const char *message){
	// get output buffer
	uint16 messagelen = (uint16)strlen(message);
	uint8 *outbuffer;

	/*
	data_write_byte(&writer, 0x0A);
	data_write_str(&writer, "hello");
	*/
	encode_u8(outbuffer + 8, 0x0A);
	encode_u16_le(outbuffer + 9, messagelen);
	memcpy(outbuffer + 11, message, messagelen);



	// connection_send

	struct data_writer writer;
	// get output buffer, setup writer
	data_write_byte(&writer, 0x0A);
	data_write_str(&writer, message);
	writer_end(&writer, xtea);
	// send buffer
}

static void login_resolve(void *handle){
	login_handle_t *h = handle;
	struct data_writer writer;
	struct account acc;
	uint32 xtea[4];

	// copy data to the stack
	memcpy(&xtea[0], &h->xtea[0], sizeof(h->xtea));
	memcpy(&acc, &h->account, sizeof(h->account));

	// send disconnect message
	data_writer_init(&writer, h->buffer, LOGIN_BUFFER_SIZE);
	writer_begin(&writer);

	/*
	data_write_byte(&writer, 0x0A);
	data_write_str(&writer, "hello");
	*/

	// @TODO: with the account information, send a request to the database thread
	// and wait for the data to be available before sending the character list
	// or login error (this will avoid stalling the network thread, in which
	// protocol handles run on, while waiting for the database read)

	// motd
	data_write_byte(&writer, 0x14);
	data_write_str(&writer, "1\nhello");
	// character list
	data_write_byte(&writer, 0x64);
	data_write_byte(&writer, 1);
	data_write_str(&writer, "Harambe");
	data_write_str(&writer, "Isara");
	data_write_u32(&writer, 16777343); // localhost
	data_write_u16(&writer, 7171);
	data_write_u16(&writer, 1);

	writer_end(&writer, xtea);
	ASSERT(connection_send(c, writer.base,
		(uint32)(writer.ptr - writer.base)));
	return PROTO_CLOSE;
}
#endif // 0
