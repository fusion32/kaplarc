#include "protocol.h"
#include "buffer_util.h"
#include "config.h"
#include "hash.h"
#include "mem.h"
#include "server.h"
#include "packed_data.h"
#include "tibia_rsa.h"
#include "crypto/xtea.h"

/* OUTPUT BUFFER */
#define LOGIN_BUFFER_CACHE CACHE512
#define LOGIN_BUFFER_SIZE 512

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool create_handle(struct connection *c, void **handle);
static void destroy_handle(struct connection *c, void *handle);
static void on_close(struct connection *c, void *handle);
static protocol_status_t on_connect(
	struct connection *c, void *handle);
static protocol_status_t on_write(
	struct connection *c, void *handle);
static protocol_status_t on_recv_message(
	struct connection *c, void *handle,
	uint8 *data, uint32 datalen);
static protocol_status_t on_recv_first_message(
	struct connection *c, void *handle,
	uint8 *data, uint32 datalen);
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

static bool create_handle(struct connection *c, void **handle){
	*handle = mem_alloc(LOGIN_BUFFER_CACHE);
	return true;
}

static void destroy_handle(struct connection *c, void *handle){
	mem_free(LOGIN_BUFFER_CACHE, handle);
}

static void on_close(struct connection *c, void *handle){
}

static protocol_status_t
on_connect(struct connection *conn, void *handle){
	return PROTO_OK;
}

static protocol_status_t
on_write(struct connection *conn, void *handle){
	return PROTO_OK;
}

static protocol_status_t
on_recv_message(struct connection *c, void *handle,
		uint8 *data, uint32 datalen){
	return PROTO_OK;
}

static void writer_begin(struct data_writer *writer){
	// skip 8 bytes:
	//  2 bytes - message length
	//  4 bytes - checksum
	//  2 bytes - unencrypted length
	writer->ptr = writer->base + 8;
}

/*
static void data_write_padding(struct data_writer *writer, int padding){
	switch(padding){
	case 7:	data_write_byte(writer, 0x33);
	case 6:	data_write_u16(writer, 0x3333);
		data_write_u32(writer, 0x33333333);
		break;
	case 5:	data_write_byte(writer, 0x33);
	case 4:	data_write_u32(writer, 0x33333333);
		break;
	case 3: data_write_byte(writer, 0x33);
	case 2: data_write_u16(writer, 0x3333);
		break;
	case 1:	data_write_byte(writer, 0x33);
		break;
	case 0:
	default:
		break;
	}
}
*/

static void writer_end(struct data_writer *writer, uint32 *xtea){
	DEBUG_ASSERT(writer != NULL);
	DEBUG_ASSERT(xtea != NULL);

	// NOTE1: both the message length and unencrypted
	// length don't include their own size
	// NOTE2: the unencrypted length is encrypted
	// together with the data

	uint8 *database = writer->base + 6;
	uint32 datalen = (uint32)(writer->ptr - database);
	uint32 checksum;
	int padding = (8 - (datalen & 7)) & 7;

	// set unencrypted length (datalen - 2)
	encode_u16_le(database, datalen-2);
	// add padding for xtea encoding
	datalen += padding;
	while(padding-- > 0)
		data_write_byte(writer, 0x33);
	// xtea encode
	xtea_encode(xtea, database, datalen);

	// calc checksum
	checksum = adler32(database, datalen);
	// set message length (datalen + 6 - 2)
	encode_u16_le(writer->base, datalen+4);
	// set checksum
	encode_u32_le(writer->base+2, checksum);
}

static protocol_status_t
on_recv_first_message(struct connection *c, void *handle,
		uint8 *data, uint32 datalen){
	size_t decoded_len;
	struct data_reader reader;
	struct data_writer writer;
	uint16 version;
	uint32 xtea[4];
	char accname[32];
	char password[32];

	// 4 bytes -> checksum
	// 1 byte -> protocol id
	// 2 bytes -> client os
	// 2 bytes -> client version
	// 12 bytes -> ?
	// 128 bytes -> rsa encoded data
	if(datalen != 149){
		DEBUG_LOG("protocol_login: invalid login message length"
			" (expected = %d, got = %d)", 149, datalen);
		return PROTO_ABORT;
	}

	data_reader_init(&reader, data, datalen);
	reader.ptr += 7; // skip checksum, protocol id, and client os
	version = data_read_u16(&reader);
	reader.ptr += 12; // skip 12 unknown bytes

	// version <= 760 didn't have encryption or checksum

	// rsa decode
	if(!tibia_rsa_decode(reader.ptr,
	  reader.end - reader.ptr, &decoded_len))
		return PROTO_ABORT;
	reader.end = reader.ptr + decoded_len;

	// xtea key
	xtea[0] = data_read_u32(&reader);
	xtea[1] = data_read_u32(&reader);
	xtea[2] = data_read_u32(&reader);
	xtea[3] = data_read_u32(&reader);

	// account credentials
	data_read_str(&reader, accname, 32);
	data_read_str(&reader, password, 32);

	DEBUG_LOG("xtea = {%08X, %08X, %08X, %08X}",
		xtea[0], xtea[1], xtea[2], xtea[3]);
	DEBUG_LOG("account name = `%s`", accname);
	DEBUG_LOG("account password = `%s`", password);

	// send disconnect message
	data_writer_init(&writer, handle, LOGIN_BUFFER_SIZE);
	writer_begin(&writer);

	/*
	data_write_byte(&writer, 0x0A);
	data_write_str(&writer, "hello");
	*/

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

/*
	auto output = output_message(256);
		message_begin(output.get());
		output->add_byte(0x0A);
		output->add_str(message);
		message_end(output.get());
		connection_send(connection, std::move(output));


	auto output = output_message(256);
		message_begin(output.get());

		// send motd
		char buf[256];
		StringPtr motd = {256, 0, buf};
		str_format(&motd, "%d\n%s",
			config_geti("motd_num"),
			config_get("motd_message"));
		output->add_byte(0x14);					// motd identifier
		output->add_lstr(motd.data, motd.len);			// motd string

		// send character list
		output->add_byte(0x64);					// character list identifier
		output->add_byte(1);					// character count
		// for(...){
			output->add_str("Jimmy");			// character name
			output->add_str("World");			// character world
			output->add_u32(0);				// world address
			output->add_u16(config_geti("sv_game_port"));	// world port
		// }
		output->add_u16(1);					// premium days left

		message_end(output.get());
*/
