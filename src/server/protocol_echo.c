#include "server.h"
#include "../buffer_util.h"
#include "../system.h"
#include <string.h>

// protocol handle
#define ECHO_BUFFER_SIZE 1024
struct echo_handle{
	bool output_ready;
	uint8 output_buffer[ECHO_BUFFER_SIZE];
};

// protocol callbacks
static bool identify(uint8 *data, uint32 datalen);
static void *create_handle(struct connection *c);
static void destroy_handle(struct connection *c, void *handle);
static void on_connect(struct connection *c, void *handle);
static void on_close(struct connection *c, void *handle);
static void on_recv_message(struct connection *c,
		void *handle, uint8 *data, uint32 datalen);
static void on_recv_first_message(struct connection *c,
		void *handle, uint8 *data, uint32 datalen);
static void on_write(struct connection *c, void *handle);

// protocol definition
struct protocol protocol_echo = {
	.name =				"ECHO",
	.sends_first =			false,
	.identify =			identify,

	.create_handle =		create_handle,
	.destroy_handle =		destroy_handle,

	.on_connect =			on_connect,
	.on_close =			on_close,
	.on_recv_message =		on_recv_message,
	.on_recv_first_message =	on_recv_first_message,
	.on_write =			on_write,
};

// protocol callbacks
static bool identify(uint8 *data, uint32 datalen){
	return datalen >= 4 &&
		data[0] == 'E' && data[1] == 'C' &&
		data[2] == 'H' && data[3] == 'O';
}

static void *create_handle(struct connection *c){
	struct echo_handle *h =
		sys_malloc(sizeof(struct echo_handle));
	h->output_ready = true;
	return h;
}

static void destroy_handle(struct connection *c, void *handle){
	sys_free(handle);
}

static void on_connect(struct connection *c, void *handle){
	// no op
}

static void on_close(struct connection *c, void *handle){
	// no op
}

static void on_recv_message(struct connection *c,
		void *handle, uint8 *data, uint32 datalen){
	struct echo_handle *h = handle;
	uint32 output_length;
	if(h->output_ready){
		output_length = MIN(datalen, ECHO_BUFFER_SIZE - 2);
		encode_u16_be(h->output_buffer, output_length);
		memcpy(h->output_buffer + 2, data, output_length);
		if(connection_send(c, h->output_buffer, output_length + 2))
			h->output_ready = false;
	}
}

static void on_recv_first_message(struct connection *c,
		void *handle, uint8 *data, uint32 datalen){
	// skip protocol identifier
	on_recv_message(c, handle, data+4, datalen-4);
}

static void on_write(struct connection *c, void *handle){
	struct echo_handle *h = handle;
	h->output_ready = true;
}
