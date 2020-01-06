#if 0

#include "protocol_test.h"

#include "message.h"
#include "outputmessage.h"
#include "server.h"

#include "../crypto/adler32.h"
#include "../log.h"

// message helpers
static bool message_checksum(Message *msg);
static void message_begin(Message *msg);
static void message_end(Message *msg);

// protocol callbacks
static bool identify(Message *first);
static void *create_handle(Connection *conn);
static void destroy_handle(Connection *conn, void *handle);
static void on_connect(Connection *conn, void *handle);
static void on_close(Connection *conn, void *handle);
static void on_recv_message(Connection *conn, void *handle, Message *msg);
static void on_recv_first_message(Connection *conn, void *handle, Message *msg);

// protocol definition
Protocol protocol_test = {
	/* .name = */			"TEST",
	/* .sends_first = */		true,
	/* .identify = */		identify,

	/* .create_handle = */		create_handle,
	/* .destroy_handle = */		destroy_handle,

	/* .on_connect = */		on_connect,
	/* .on_close = */		on_close,
	/* .on_recv_message = */	on_recv_message,
	/* .on_recv_first_message = */	on_recv_first_message,
};


// message helpers
static bool message_checksum(Message *msg){
	size_t offset = msg->readpos + 4;
	uint32 sum = adler32(msg->buffer + offset, msg->length - offset);
	return sum == msg->peek_u32();
}
static void message_begin(Message *msg){
	msg->readpos = 2;
	msg->length = 0;
}
static void message_end(Message *msg){
	msg->readpos = 2;
	msg->radd_u16((uint16)msg->length);
}


// protocol callbacks
static bool identify(Message *first){
	protocol_test.on_connect = NULL;
	protocol_test.on_close = NULL;

	size_t offset = first->readpos;
	// skip checksum
	if(message_checksum(first))
		offset += 4;
	// check protocol identifier
	return first->buffer[offset] == 0xA3;
}


static void *create_handle(Connection *conn){
	return NULL;
}

static void destroy_handle(Connection *conn, void *handle){
}

static void send_hello(Connection *conn){
	OutputMessage outputmsg;
	Message *msg;

	output_message_acquire(MESSAGE_CAPACITY_256, &outputmsg);
	msg = outputmsg.msg;

	message_begin(msg);
	msg->add_lstr("Hello World", 11);
	message_end(msg);
	connection_send(conn, &outputmsg);

}

static void on_connect(Connection *conn, void *handle){
	LOG("on_connect");
	send_hello(conn);
}

static void on_close(Connection *conn, void *handle){
	LOG("on_close");
}

static void on_recv_message(Connection *conn, void *handle, Message *msg){
	char buf[64];
	if(message_checksum(msg))
		msg->readpos += 4;
	msg->get_str(buf, 64);
	LOG("on_recv_message: %s", buf);
	send_hello(conn);
}

static void on_recv_first_message(Connection *conn, void *handle, Message *msg){
	char buf[64];
	if(message_checksum(msg))
		msg->readpos += 4;
	if(msg->get_byte() != 0xA3)
		UNREACHABLE();
	msg->get_str(buf, 64);
	LOG("on_recv_first_message: %s", buf);
	send_hello(conn);
}

#endif
