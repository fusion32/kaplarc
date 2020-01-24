#if 0
#include "server.h"

#include "../crypto/adler32.h"
#include "../packed_data.h"
#include "../dispatcher.h"
#include "../log.h"

// message helpers
static bool message_checksum(struct packed_data *msg);
static void message_begin(struct packed_data *msg);
static void message_end(struct packed_data *msg);

// protocol callbacks
static bool identify(struct packed_data *first);
static void *create_handle(struct connection *c);
static void destroy_handle(struct connection *c, void *handle);
static void on_connect(struct connection *c, void *handle);
static void on_close(struct connection *c, void *handle);
static void on_recv_message(struct connection *c,
		void *handle, struct packed_data *msg);
static void on_recv_first_message(struct connection *c,
		void *handle, struct packed_data *msg);

// protocol definition
struct protocol protocol_test = {
	.name =				"TEST",
	.sends_first =			true,
	.identify =			identify,

	.create_handle =		create_handle,
	.destroy_handle =		destroy_handle,

	.on_connect =			on_connect,
	.on_close =			on_close,
	.on_recv_message =		on_recv_message,
	.on_recv_first_message =	on_recv_first_message,
};


// message helpers
static bool message_checksum(struct packed_data *msg){
	size_t offset = msg->pos + 4;
	uint32 sum = adler32(msg->base + offset, msg->length - offset);
	return sum == pd_peek_u32(msg);
}
static void message_begin(struct packed_data *msg){
	msg->pos = 2;
	msg->length = 0;
}
static void message_end(struct packed_data *msg){
	msg->pos = 2;
	pd_radd_u16(msg, (uint16)msg->length);
}


// protocol callbacks
static bool identify(struct packed_data *first){
	size_t offset = first->pos;
	// skip checksum
	if(message_checksum(first))
		offset += 4;
	// check protocol identifier
	return first->base[offset] == 0xA3;
}


static void *create_handle(struct connection *c){
	return NULL;
}

static void destroy_handle(struct connection *c, void *handle){
}

static void __send_hello(void *data){
	struct connection *c = data;
	struct packed_data msg;
	//connection_get_output_buffer(c, &msg, &len);
	message_begin(&msg);
	pd_add_lstr(&msg, "Hello World", 11);
	message_end(&msg);
}

static void send_hello(struct connection *c){
	dispatcher_add(__send_hello, c);
}

static void on_connect(struct connection *c, void *handle){
	LOG("on_connect");
	send_hello(c);
}

static void on_close(struct connection *c, void *handle){
	LOG("on_close");
}

static void on_recv_message(struct connection *c,
		void *handle, struct packed_data *msg){
	char buf[64];
	if(message_checksum(msg))
		msg->pos += 4;
	pd_get_str(msg, buf, 64);
	LOG("on_recv_message: %s", buf);
	send_hello(c);
}

static void on_recv_first_message(struct connection *c,
		void *handle, struct packed_data *msg){
	char buf[64];
	if(message_checksum(msg))
		msg->pos += 4;
	if(pd_get_byte(msg) != 0xA3)
		UNREACHABLE();
	pd_get_str(msg, buf, 64);
	LOG("on_recv_first_message: %s", buf);
	send_hello(c);
}
#endif //0
