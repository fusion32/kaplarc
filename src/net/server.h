#ifndef SERVER_H_
#define SERVER_H_

#include "../def.h"

class Connection;
class Message;
class Service;

#define PROTOCOL_SENDS_FIRST 0x01
class Protocol{
public:
	// protocol information
	static constexpr char	*name = "none";
	static constexpr uint32	identifier = 0x00;
	static constexpr uint32	flags = 0;

	// protocol interface
	virtual		~Protocol(void) = 0;
	virtual void	message_begin(Message *msg) = 0;
	virtual void	message_end(Message *msg) = 0;
	virtual void	on_connect(void) = 0;
	virtual void	on_recv_message(Message *msg) = 0;
	virtual void	on_recv_first_message(Message *msg) = 0;
};

template<typename T>
void server_add_protocol(int port);
void server_run(void);
void server_stop(void);
Protocol *server_make_protocol(Service *service,
	Connection *conn, uint32 identifier);

#endif //SERVER_H_
