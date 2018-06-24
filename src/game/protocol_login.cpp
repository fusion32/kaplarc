#include "protocol_login.h"

#include "../config.h"
#include "../crypto/adler32.h"
#include "../crypto/xtea.h"
#include "../dispatcher.h"
#include "../log.h"
#include "../server/connection.h"
#include "../server/message.h"
#include "../server/outputmessage.h"
#include "../sstring.h"
#include "srsa.h"

// helpers for this protocol
static uint32 checksum(Message *msg){
	size_t offset = msg->readpos + 4;
	return adler32(msg->buffer + offset, msg->length - offset);
}
static void message_begin(Message *msg, uint32 *xtea, bool checksum_enabled){
	msg->readpos = checksum_enabled ? 6 : 2;
	msg->length = 0;
}
static void message_end(Message *msg, uint32 *xtea, bool checksum_enabled){
	int padding;
	size_t start = checksum_enabled ? 6 : 2;
	if(xtea != nullptr){
		padding = msg->length % 8;
		while(padding-- > 0)
			msg->add_byte(0x33);
		xtea_encode(xtea, msg->buffer + start, msg->length);
	}
	if(checksum_enabled){
		msg->readpos = 6;
		msg->radd_u32(checksum(msg));
	}else{
		msg->readpos = 2;
	}
	msg->radd_u16((uint16)msg->length);
}

void ProtocolLogin::disconnect(const char *message, uint32 *xtea, bool checksum_enabled){
	auto output = output_message(256);
	if(output != nullptr){
		message_begin(output.get(), xtea, checksum_enabled);
		output->add_byte(0x0A);
		output->add_str(message);
		message_end(output.get(), xtea, checksum_enabled);
		connmgr_send(connection, std::move(output));
	}
	connmgr_close(connection);
}


// protocol implementation
bool ProtocolLogin::identify(Message *first){
	size_t offset = first->readpos;
	if(first->peek_u32() == checksum(first))
		offset += 4;
	return first->buffer[offset] == 0x01;
}

ProtocolLogin::ProtocolLogin(const std::shared_ptr<Connection> &conn)
  : Protocol(conn) {
}

ProtocolLogin::~ProtocolLogin(void){
}

void ProtocolLogin::on_recv_first_message(Message *msg){
	uint16 system;
	uint16 version;
	uint32 key[4];
	char account[64];
	char password[256];

	bool checksum_enabled = (msg->peek_u32() == checksum(msg));
	if(checksum_enabled) msg->readpos += 4;

	if(msg->get_byte() != 0x01){
		disconnect("internal error", nullptr, checksum_enabled);
		return;
	}

	system = msg->get_u16();
	version = msg->get_u16();

	if(version <= 760){
		disconnect("This server requires client version 8.6", nullptr, checksum_enabled);
		return;
	}

	msg->readpos += 12;
	if(!srsa_decode((char*)(msg->buffer + msg->readpos),
			(msg->length - msg->readpos), nullptr)){
		disconnect("internal error", nullptr, checksum_enabled);
		return;
	}

	key[0] = msg->get_u32();
	key[1] = msg->get_u32();
	key[2] = msg->get_u32();
	key[3] = msg->get_u32();

	msg->get_str(account, 64);
	msg->get_str(password, 64);

	if(account[0] == 0){
		disconnect("Invalid account name", key, checksum_enabled);
		return;
	}

	// authenticate user
	if(true){
		disconnect("Invalid account name and password combination", key, checksum_enabled);
		return;
	}

	auto output = output_message(256);
	if(output){
		message_begin(output.get(), key, checksum_enabled);

		// send motd
		SString<256> motd;
		motd.format("%d\n", config_geti("motd_num"));
		motd.append(config_get("motd_message"));
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

		message_end(output.get(), key, checksum_enabled);
		connmgr_send(connection, std::move(output));
	}

	// close connection
	connmgr_close(connection);
}
