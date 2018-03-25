#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "../def.h"
#include "../log.h"

#include <atomic>
#include <memory>

class Connection;
class Message;

/*************************************

	Protocol Interface

*************************************/
class Protocol
 : public std::enable_shared_from_this<Protocol>{
protected:
	std::shared_ptr<Connection> connection;

public:
	// protocol information
	static constexpr char	*name = "none";
	static constexpr uint32	identifier = 0x00;
	static constexpr bool	single = false;

	// delete default constructor
	Protocol(void) = delete;
	// delete move operations
	Protocol(Protocol&&) = delete;
	Protocol &operator=(Protocol&&) = delete;
	// delete copy operations
	Protocol(const Protocol&) = delete;
	Protocol &operator=(const Protocol&) = delete;

	// construct from a connection
	Protocol(const std::shared_ptr<Connection> &conn)
		: connection(conn) {}

	// protocol interface
	virtual		~Protocol(void) { LOG("protocol released"); }
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
	virtual std::shared_ptr<Protocol>
	make_protocol(const std::shared_ptr<Connection> &conn) = 0;
};

template <typename T>
class ProtocolFactory: public IProtocolFactory{
public:
	virtual const char *name(void) override{ return T::name; }
	virtual const uint32 identifier(void) override{ return T::identifier; }
	virtual const bool single(void) override{ return T::single; }
	virtual std::shared_ptr<Protocol>
	make_protocol(const std::shared_ptr<Connection> &conn) override{
		return std::static_pointer_cast<Protocol>(
			std::make_shared<T>(conn));
	}
};

#endif //PROTOCOL_H_
