#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "../def.h"
#include "../log.h"

#include <memory>

class Connection;
class Message;

/*			Protocol Guidelines

 1  -	Every protocol must have the default constructor deleted and have a
	custom constructor defined as:
		ProtocolName(const std::shared_ptr<Connection> &conn);

 2  -	Protocol callbacks may not call connmgr_accept, connmgr_close, or
	connmgr_send directly as doing so would cause a deadlock situation.

 3a -	When the connection is released, it will immediatelly inform the protocol
	with a call to the close callback. To avoid a problem with the protocol
	being released in the middle of an operation, it has an internal reference
	to itself that must be passed around into the operations (asynchronous
	operations).

 3b -	The `release` method must be called from within the close callback as
	there would be no other oportunity to release the protocol.

 3c -	Protocol callbacks are assured to be run in serial by the connection so no
	internal synchronization is needed
*/

/*************************************

	Protocol Interface

*************************************/
class Protocol{
protected:
	std::shared_ptr<Connection>	connection;
	std::shared_ptr<Protocol>	self;

public:
	// protocol information
	static constexpr char	*name = "none";
	static constexpr uint32	identifier = 0x00;
	static constexpr bool	single = false;

	// delete default, copy, and move constructors
	Protocol(void) = delete;
	Protocol(const Protocol&) = delete;
	Protocol(Protocol&&) = delete;

	Protocol(const std::shared_ptr<Connection> &conn)
		: connection(conn), self(this) {}
	void release(void) { self.reset(); }

	// protocol interface
	virtual		~Protocol(void) {
		LOG("protocol released");
	}
	virtual void	on_connect(void) = 0;
	virtual void	on_close(void) = 0;
	virtual void	on_recv_message(Message *msg) = 0;
	virtual void	on_recv_first_message(Message *msg) = 0;
};


/*************************************

	Protocol Factory

*************************************/
class IProtocolFactory{
public:
	virtual ~IProtocolFactory(void) {};
	virtual const char *name(void) = 0;
	virtual const uint32 identifier(void) = 0;
	virtual const bool single(void) = 0;
	virtual Protocol *make_protocol(const std::shared_ptr<Connection> &conn) = 0;
};

template <typename T>
class ProtocolFactory: public IProtocolFactory{
public:
	virtual const char *name(void) override{ return T::name; }
	virtual const uint32 identifier(void) override{ return T::identifier; }
	virtual const bool single(void) override{ return T::single; }
	virtual Protocol *make_protocol(const std::shared_ptr<Connection> &conn) override{
		return new T(conn);
	}
};

#endif //PROTOCOL_H_
