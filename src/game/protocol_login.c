#include "../buffer_util.h"
#include "../config.h"
#include "../hash.h"
#include "../slab_cache.h"
#include "../server/server.h"
#include "packed_data.h"
#include "tibia_rsa.h"

/* OUTPUT BUFFERS */
#define BUFFER_MAX_SIZE 256
#define BUFFERS_PER_SLAB 32
static struct slab_cache *buffers;

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool init(void);
static void shutdown(void);
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

	.init = init,
	.shutdown = shutdown,
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
	uint32 checksum;
	// check if message contains checksum
	if(datalen > 4){
		checksum = adler32(data+4, datalen-4);
		if(checksum == decode_u32_le(data)){
			data += 4;
			datalen -= 4;
		}
	}
	// check protocol login identifier
	return datalen > 0 && data[0] == 0x01;
}

static bool init(void){
	buffers = slab_cache_create(
		BUFFERS_PER_SLAB, BUFFER_MAX_SIZE);
	return buffers != NULL;
}

static void shutdown(void){
	slab_cache_destroy(buffers);
}

static bool create_handle(struct connection *c, void **handle){
	*handle = slab_cache_alloc(buffers);
	return *handle != NULL;
}

static void destroy_handle(struct connection *c, void *handle){
	slab_cache_free(buffers, handle);
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

#include <stdio.h>
static void data_dump(uint8 *data, size_t datalen){
	DEBUG_LOG("data_dump:");
	for(uint32 i = 0; i < datalen; i += 1){
		if((i & 0x0F) == 0)
			printf("\n\t%02X", data[i]);
		else
			printf(" %02X", data[i]);
	}
	printf("\n");
}
static protocol_status_t
on_recv_first_message(struct connection *c, void *handle,
		uint8 *data, uint32 datalen){
	size_t decoded_len;
	struct data_reader reader;

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
	data_read_u32(&reader); // skip checksum
	data_read_byte(&reader); // skip protocol id
	data_read_u16(&reader); // client os
	data_read_u16(&reader); // client version
	// skip 12 unknown bytes
	reader.ptr += 12;

	// rsa decode
	if(!tibia_rsa_decode(reader.ptr,
	  reader.end - reader.ptr, &decoded_len))
		return PROTO_ABORT;
	reader.end = reader.ptr + decoded_len;
	data_dump(reader.ptr, decoded_len);
	return PROTO_CLOSE;
}

/*
class ProtocolLogin: public Protocol{
private:
	// internal data
	bool use_checksum;
	bool use_xtea;
	uint32 xtea[4];
};

// helpers for this protocol
static uint32 calc_checksum(Message *msg){
	size_t offset = msg->readpos + 4;
	return adler32(msg->buffer + offset, msg->length - offset);
}
void ProtocolLogin::message_begin(Message *msg){
	msg->readpos = use_checksum ? 6 : 2;
	msg->length = 0;
}
void ProtocolLogin::message_end(Message *msg){
	int padding;
	size_t start = use_checksum ? 6 : 2;
	if(use_xtea){
		padding = msg->length % 8;
		while(padding-- > 0)
			msg->add_byte(0x33);
		xtea_encode(xtea, msg->buffer + start, msg->length);
	}
	if(use_checksum){
		msg->readpos = 6;
		msg->radd_u32(calc_checksum(msg));
	}else{
		msg->readpos = 2;
	}
	msg->radd_u16((uint16)msg->length);
}

void ProtocolLogin::disconnect(const char *message){
	auto output = output_message(256);
	if(output != NULL){
		message_begin(output.get());
		output->add_byte(0x0A);
		output->add_str(message);
		message_end(output.get());
		connection_send(connection, std::move(output));
	}
	connection_close(connection);
}


// protocol implementation
bool ProtocolLogin::identify(Message *first){
	size_t offset = first->readpos;
	if(first->peek_u32() == calc_checksum(first))
		offset += 4;
	return first->buffer[offset] == 0x01;
}

ProtocolLogin::ProtocolLogin(Connection *conn)
  : Protocol(conn), use_checksum(false), use_xtea(false) {
}

ProtocolLogin::~ProtocolLogin(void){
}

void ProtocolLogin::on_recv_first_message(Message *msg){
	uint16 system;
	uint16 version;
	char account[64];
	char password[256];

	// verify checksum
	use_checksum = (msg->peek_u32() == calc_checksum(msg));
	if(use_checksum)
		msg->readpos += 4;

	// verify protocol identifier
	if(msg->get_byte() != 0x01){
		disconnect("internal error");
		return;
	}

	system = msg->get_u16();
	version = msg->get_u16();
	if(version <= 760){
		disconnect("This server requires client version 8.6");
		return;
	}

	msg->readpos += 12;
	if(!grsa_decode(msg->buffer + msg->readpos,
			(msg->length - msg->readpos), NULL)){
		disconnect("internal error");
		return;
	}

	// enable encryption
	xtea[0] = msg->get_u32();
	xtea[1] = msg->get_u32();
	xtea[2] = msg->get_u32();
	xtea[3] = msg->get_u32();
	use_xtea = true;

	msg->get_str(account, 64);
	msg->get_str(password, 64);

	if(account[0] == 0){
		disconnect("Invalid account name");
		return;
	}

	// authenticate user
	if(true){
		disconnect("Invalid account name and password combination");
		return;
	}

	auto output = output_message(256);
	if(output){
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
		connection_send(connection, std::move(output));
	}

	// close connection
	connection_close(connection);
}
*/
