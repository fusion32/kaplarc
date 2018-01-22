#include "connection.h"
#include "message.h"
#include "protocol_test.h"
#include "server.h"
#include "../log.h"

ProtocolTest::ProtocolTest(const std::shared_ptr<Connection> &conn)
  : connection(conn) {}

ProtocolTest::~ProtocolTest(void){
}

void ProtocolTest::message_begin(Message *msg){
	msg->readpos = 2;
	msg->length = 0;
}

void ProtocolTest::message_end(Message *msg){
	msg->readpos = 2;
	msg->radd_u16((uint16)msg->length);
}

void ProtocolTest::on_connect(void){
	LOG("on_connect");
	work_dispatch([this](void){
		send_hello(); });
}

void ProtocolTest::on_close(void){
	LOG("on_close");
	connection.reset();
}

void ProtocolTest::on_recv_message(Message *msg){
	char buf[64];
	msg->get_str(buf, 64);
	LOG("on_recv_message: %s", buf);
	work_dispatch([this](void){
		send_hello(); });
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
