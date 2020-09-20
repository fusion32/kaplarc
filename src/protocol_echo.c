#include "common.h"
#include "buffer_util.h"
#include "server/server.h"
#include <string.h>

/* PROTOCOL HANDLE */
#define ECHO_BUFFER_SIZE (1024 - sizeof(bool))
struct echo_handle{
	bool output_ready;
	uint8 output_buffer[ECHO_BUFFER_SIZE];
};

/* IMPL START */
static bool identify(uint8 *data, uint32 datalen){
	return datalen >= 4 &&
		data[0] == 'E' && data[1] == 'C' &&
		data[2] == 'H' && data[3] == 'O';
}

static bool on_assign_protocol(uint32 c){
	struct echo_handle *h = kpl_malloc(sizeof(struct echo_handle));
	h->output_ready = true;
	*connection_userdata(c) = h;
	return true;
}

static void on_close(uint32 c){
	kpl_free(*connection_userdata(c));
}

static protocol_status_t on_write(uint32 c){
	struct echo_handle *h = *connection_userdata(c);
	h->output_ready = true;
	return PROTO_OK;
}

static protocol_status_t on_recv_message(uint32 c, uint8 *data, uint32 datalen){
	struct echo_handle *h = *connection_userdata(c);
	uint32 output_length;
	if(h->output_ready){
		output_length = MIN(datalen, ECHO_BUFFER_SIZE - 2);
		encode_u16_le(h->output_buffer, output_length);
		memcpy(h->output_buffer + 2, data, output_length);
		if(connection_send(c, h->output_buffer, output_length + 2))
			h->output_ready = false;
	}
	return PROTO_OK;
}

static protocol_status_t on_recv_first_message(uint32 c, uint8 *data, uint32 datalen){
	// skip protocol identifier and parse as a regular message
	return on_recv_message(c, data+4, datalen-4);
}

/* PROTOCOL DECL */
struct protocol protocol_echo = {
	.name =				"ECHO",
	.sends_first =			false,
	.identify =			identify,

	.on_assign_protocol =		on_assign_protocol,
	.on_close =			on_close,
	.on_connect =			NULL,
	.on_write =			on_write,
	.on_recv_message =		on_recv_message,
	.on_recv_first_message =	on_recv_first_message,
};
