#include "connection.h"
#include "message.h"
#include "protocol_test.h"
#include "server.h"
#include "../log.h"
#include "../dispatcher.h"

// message helpers for sending a message over this protocol
static void message_begin(Message *msg){
	msg->readpos = 2;
	msg->length = 0;
}
static void message_end(Message *msg){
	msg->readpos = 2;
	msg->radd_u16((uint16)msg->length);
}

// protocol implementation
ProtocolTest::ProtocolTest(const std::shared_ptr<Connection> &conn)
	: Protocol(conn) {
}

ProtocolTest::~ProtocolTest(void){
}

// !!! TODO: figure how to make this more feasible
// (this is where C pointers would make it simpler)
#define CAST_TO_SELF(x)		std::static_pointer_cast<ProtocolTest>(x)
#define CAST_TO_SELF_REF(x)	static_cast<ProtocolTest&>(*(x))

void ProtocolTest::on_connect(void){
	LOG("on_connect");
	dispatcher_add([p = self](void)
		{ CAST_TO_SELF(p)->send_hello(); });
}

void ProtocolTest::on_close(void){
	release();
}

void ProtocolTest::on_recv_message(Message *msg){
	char buf[64];
	msg->get_str(buf, 64);
	dispatcher_add([p = self](void)
		{ CAST_TO_SELF(p)->send_hello(); });
}

void ProtocolTest::on_recv_first_message(Message *msg){
	LOG("on_recv_first_message: %d", msg->get_byte());
	on_recv_message(msg);
}

void ProtocolTest::send_hello(void){
	Message *msg = output_pool_acquire(MSG_CAPACITY_SMALL);
	if(msg != nullptr){
		message_begin(msg);
		msg->add_str("Hello World", 11);
		message_end(msg);
		connmgr_send(connection, msg);
	}
}
