#ifndef SERVER_H_
#define SERVER_H_

#include "../def.h"
#include "protocol.h"
class Connection;

class Service{
private:
	// service control
	std::vector<IProtocolFactory*>	factories;
	Socket				*socket;
	int				port;

	// helpers
	bool open(void);
	void close(void);
	static void on_accept(Socket *sock, int error, int transfered, void *udata);

	// give control to the server
	friend class Server;

public:
	Service(int port_);
	~Service(void);

	// service information
	int get_port(void) const;
	bool single_protocol(void) const;

	// protocol management
	template<typename T>
	bool		add_protocol(void);
	Protocol	*make_protocol(Connection *conn, uint32 identifier);
};

class Server{
private:
	// server control
	std::vector<Service*>	services;
	bool			running;

	// delete copy and move operations
	Server(Server&) = delete;
	Server(Server&&) = delete;
	Server &operator=(Server&) = delete;
	Server &operator=(Server&&) = delete;

	// private construtor and destrutor
	Server(void);
	~Server(void);

public:
	static Server &instance(void){
		static Server instance;
		return instance;
	}
	template<typename T>
	bool add_protocol(int port);
	void run(void);
	void stop(void);
	Protocol *make_protocol(Service *service,
		Connection *conn, uint32 identifier);
};

#endif //SERVER_H_
