#ifndef SERVER_H_
#define SERVER_H_

#include "../def.h"
#include "protocol.h"
#include <memory>
#include <vector>

class Connection;

class Service{
private:
	std::vector<IProtocolFactory*>	factories;
	Socket				*socket;
	int				port;

	bool open(void);
	void close(void);
	static void on_accept(Socket *sock, int error, int transfered, Service *service);
	friend class Server;

public:
	Service(int port_);
	~Service(void);

	int get_port(void) const;
	bool single_protocol(void) const;
	template<typename T>
	bool add_protocol(void);
	Protocol *make_protocol(const std::shared_ptr<Connection> &conn);
	Protocol *make_protocol(const std::shared_ptr<Connection> &conn, uint32 identifier);
};

class Server{
private:
	std::vector<Service*>	services;
	bool			running;

	Server(Server&) = delete;
	Server(Server&&) = delete;
	Server &operator=(Server&) = delete;
	Server &operator=(Server&&) = delete;

	Server(void);
	~Server(void);

public:
	static Server *instance(void){
		static Server instance;
		return &instance;
	}
	template<typename T>
	bool add_protocol(int port);
	void run(void);
	void stop(void);
};

#endif //SERVER_H_
