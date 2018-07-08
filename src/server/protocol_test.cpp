#include "protocol_test.h"

#include "connection.h"
#include "message.h"
#include "outputmessage.h"
#include "../crypto/adler32.h"
#include "../dispatcher.h"
#include "../log.h"

// message helpers for sending a message over this protocol
static bool checksum(Message *msg){
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

// protocol implementation
bool ProtocolTest::identify(Message *first){
	size_t offset = first->readpos;
	// skip checksum
	if(checksum(first))
		offset += 4;
	// check protocol identifier
	return first->buffer[offset] == 0xA3;
}

ProtocolTest::ProtocolTest(Connection *conn)
  : Protocol(conn) {
}

ProtocolTest::~ProtocolTest(void){
}

void ProtocolTest::on_connect(void){
	LOG("on_connect");
	send_hello();
}

void ProtocolTest::on_close(void){
	LOG("on_close");
}

void ProtocolTest::on_recv_message(Message *msg){
	if(checksum(msg))
		msg->readpos += 4;
	parse(msg);
}

void ProtocolTest::on_recv_first_message(Message *msg){
	if(checksum(msg))
		msg->readpos += 4;
	if(msg->get_byte() != 0xA3)
		UNREACHABLE();
	parse(msg);
}

void ProtocolTest::parse(Message *msg){
	char buf[64];
	msg->get_str(buf, 64);
	LOG("on_recv_message: %s", buf);
	send_hello();
}

void ProtocolTest::send_hello(void){
	auto msg = output_message(128);
	if(msg){
		message_begin(msg.get());
		msg->add_lstr("Hello World", 11);
		message_end(msg.get());
		connection_send(connection, std::move(msg));
	}
}
