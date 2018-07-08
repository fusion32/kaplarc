#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_

#include "../def.h"
#include "../log.h"
#include "../shared.h"
#include "connection.h"
#include <atomic>

class Connection;
class Message;

/*************************************

	Protocol Interface

*************************************/

class Protocol
  : public Shared<Protocol>{
protected:
	Connection * const connection;

public:
	// protocol information
	static constexpr char	*name = "none";
	static constexpr bool	single = false;
	static bool		identify(Message *first){
		return false;
	}

	// delete default constructor
	Protocol(void) = delete;
	// delete copy
	Protocol(const Protocol&) = delete;
	Protocol &operator=(const Protocol&) = delete;

	// construct from connection
	Protocol(Connection *conn)
		: connection(conn) {}

	// protocol interface
	virtual ~Protocol(void){
		// release internal reference of connection
		connection_decref(connection);
		LOG("protocol released");
	}
	virtual void	on_connect(void) {}
	virtual void	on_close(void) {}
	virtual void	on_recv_message(Message *msg) {}
	virtual void	on_recv_first_message(Message *msg) {}
};

/*************************************

	Protocol Factory

*************************************/
class IProtocolFactory{
public:
	virtual ~IProtocolFactory(void) {};
	virtual const char *name(void) = 0;
	virtual const bool single(void) = 0;
	virtual const bool identify(Message *first) = 0;
	virtual Protocol *make_protocol(Connection *conn) = 0;
};

template <typename T>
class ProtocolFactory: public IProtocolFactory{
public:
	virtual const char *name(void) override{ return T::name; }
	virtual const bool single(void) override{ return T::single; }
	virtual const bool identify(Message *first) override{
		return T::identify(first);
	}
	virtual Protocol *make_protocol(Connection *conn) override{
		return new T(conn);
	}
};

#endif //PROTOCOL_H_
