#if 0
#include "protocol_login.h"

#include "../config.h"
#include "../crypto/adler32.h"
#include "../crypto/xtea.h"
#include "../dispatcher.h"
#include "../log.h"
#include "../server/connection.h"
#include "../server/message.h"
#include "../server/outputmessage.h"
#include "../stackstring.h"
#include "grsa.h"

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
	if(output != nullptr){
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
			(msg->length - msg->readpos), nullptr)){
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
		StackString<256> motd;
		motd.format("%d\n%s", config_geti("motd_num"),
				config_get("motd_message"));
		output->add_byte(0x14);					// motd identifier
		output->add_lstr(motd.ptr(), motd.size());		// motd string

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
#endif

