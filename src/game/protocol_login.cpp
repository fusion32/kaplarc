#include "protocol_login.h"

#include "../config.h"
#include "../crypto/adler32.h"
#include "../crypto/xtea.h"
#include "../dispatcher.h"
#include "../log.h"
#include "../server/connection.h"
#include "../server/message.h"
#include "../sstring.h"
#include "srsa.h"

// helpers for this protocol
static uint32 adler32(Message *msg){
	uint32 offset = msg->readpos + 4;
	return adler32(msg->buffer + offset, msg->length - offset);
}
static void message_begin(Message *msg, uint32 *xtea, bool checksum){
	msg->readpos = checksum ? 6 : 2;
	msg->length = 0;
}
static void message_end(Message *msg, uint32 *xtea, bool checksum){
	int padding;
	uint32 start = checksum ? 6 : 2;
	if(xtea != nullptr){
		padding = msg->length % 8;
		while(padding-- > 0)
			msg->add_byte(0x33);
		xtea_encode(xtea, msg->buffer + start, msg->length);
	}
	if(checksum){
		msg->readpos = 6;
		msg->radd_u32(adler32(msg));
	}else{
		msg->readpos = 2;
	}
	msg->radd_u16((uint16)msg->length);
}

void ProtocolLogin::disconnect(const char *message, uint32 *xtea, bool checksum){
	Message *output = output_pool_acquire(MSG_CAPACITY_SMALL);
	if(output != nullptr){
		message_begin(output, xtea, checksum);
		output->add_byte(0x0A);
		output->add_str(message);
		message_end(output, xtea, checksum);
		connmgr_send(connection, output);
	}
	connmgr_close(connection);
}


// protocol implementation
bool ProtocolLogin::identify(Message *first){
	uint32 offset = first->readpos;
	if(first->peek_u32() == adler32(first))
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

	bool checksum = (msg->peek_u32() == adler32(msg));
	if(checksum) msg->readpos += 4;

	if(msg->get_byte() != 0x01){
		disconnect("internal error", nullptr, checksum);
		return;
	}

	system = msg->get_u16();
	version = msg->get_u16();

	if(version <= 760){
		disconnect("This server requires client version 8.6", nullptr, checksum);
		return;
	}

	msg->readpos += 12;
	if(!srsa_decode((char*)(msg->buffer + msg->readpos),
			(msg->length - msg->readpos), nullptr)){
		disconnect("internal error", nullptr, checksum);
		return;
	}

	key[0] = msg->get_u32();
	key[1] = msg->get_u32();
	key[2] = msg->get_u32();
	key[3] = msg->get_u32();

	msg->get_str(account, 64);
	msg->get_str(password, 64);

	if(account[0] == 0){
		disconnect("Invalid account name", key, checksum);
		return;
	}

	// authenticate user
	if(true){
		disconnect("Invalid account name and password combination", key, checksum);
		return;
	}

	Message *output = output_pool_acquire(MSG_CAPACITY_SMALL);
	if(output != nullptr){
		message_begin(output, key, checksum);

		// send motd
		SString<256> motd;
		motd.copyf("%d\n", config_geti("motd_num"));
		motd.append(config_get("motd_message"));
		output->add_byte(0x14);
		output->add_lstr(motd.str(), motd.size());

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

		message_end(output, key, checksum);
		connmgr_send(connection, output);
	}

	// close connection
	connmgr_close(connection);
}
