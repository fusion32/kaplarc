#ifndef SERVER_H_
#define SERVER_H_

#include "../def.h"
#include "../log.h"
#include "connection.h"
#include "protocol.h"

#include <memory>
#include <vector>

class Service{
private:
	std::vector<IProtocolFactory*>	factories;
	Socket				*socket;
	int				port;

	// delete move and copy
	Service(const Service&) = delete;
	Service(Service&&) = delete;
	Service &operator=(const Service&) = delete;
	Service &operator=(Service&&) = delete;

	bool open(void);
	void close(void);
	static void on_accept(Socket *sock, int error, int transfered,
				Service *service);
	friend class Server;

public:
	Service(int port_);
	~Service(void);

	int get_port(void) const;
	bool single_protocol(void) const;
	Protocol *make_protocol(const std::shared_ptr<Connection> &conn);
	Protocol *make_protocol(const std::shared_ptr<Connection> &conn, uint32 identifier);

	template<typename T>
	bool add_protocol(void){
		if(!factories.empty()){
			IProtocolFactory *factory = factories[0];
			if(factory->single() || T::single){
				LOG_ERROR("Service::add_protocol: protocols `%s` and `%s` cannot use the same port (%d)",
					factory->name(), T::name, port);
			}
		}
		factories.push_back(new ProtocolFactory<T>);
		return true;
	}
};


class Server{
private:
	std::vector<Service*>	services;
	bool			running;

	// delete move and copy operations
	Server(const Server&) = delete;
	Server(Server&&) = delete;
	Server &operator=(const Server&) = delete;
	Server &operator=(Server&&) = delete;

	// private constructor/destructor
	Server(void);
	~Server(void);

public:
	static Server *instance(void){
		static Server instance;
		return &instance;
	}

	void run(void);
	void stop(void);

	template<typename T>
	bool add_protocol(int port){
		if(running){
			LOG_ERROR("Server::add_protocol: server already running");
			return false;
		}

		Service *service;
		auto it = std::find_if(services.begin(), services.end(),
			[port](Service *service){ return (service->port == port); });
		if(it == services.end()){
			service = new Service(port);
			services.push_back(service);
		}else{
			service = *it;
		}
		return service->add_protocol<T>();
	}
};

#endif //SERVER_H_
