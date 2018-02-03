#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "../def.h"

#include <memory>

class Connection;
class Message;

/*			Protocol Guidelines

 1 -	Every protocol must have the default constructor deleted and have a
	custom constructor defined as:
		ProtocolName(const std::shared_ptr<Connection> &conn);

 2 -	Protocol callbacks may not call connmgr_accept, connmgr_close, or
	connmgr_send directly as doing so would cause a deadlock situation.
*/

/*************************************

	Protocol Interface

*************************************/
class Protocol{
public:
	// protocol information
	static constexpr char	*name = "none";
	static constexpr uint32	identifier = 0x00;
	static constexpr bool	single = false;

	// protocol interface
	virtual		~Protocol(void){}
	virtual void	message_begin(Message *msg) = 0;
	virtual void	message_end(Message *msg) = 0;
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
