
#include <algorithm>
#include <vector>
#include "network.h"
#include "server.h"
#include "../log.h"

/*************************************

	Protocol Factory

*************************************/
class IProtocolFactory{
public:
	virtual const char *name(void) = 0;
	virtual const uint32 identifier(void) = 0;
	virtual const uint32 flags(void) = 0;
	virtual Protocol *make_protocol(Connection *conn) = 0;
};

template <typename T>
class ProtocolFactory: IProtocolFactory{
public:
	virtual const char *name(void) override{ return T::name; }
	virtual const uint32 identifier(void) override{ return T::identifier; }
	virtual const uint32 flags(void) override{ return T::flags; }
	virtual Protocol *make_protocol(Connection *conn) override{
		return new T(conn);
	}
};


/*************************************

	Service Implementation

*************************************/
class Service{
private:
	int port;
	Socket *socket;
	std::vector<IProtocolFactory*> factories;

public:
	Service(int port_)
	  : port(port_), socket(nullptr){}

	~Service(void){
		if(socket != nullptr){
			socket_close(socket);
			socket = nullptr;
		}
		for(IProtocolFactory *factory : factories)
			delete factory;
	}

	int get_port(void) const { return port; }

	template<typename T>
	bool add_protocol(void){
		if(!factories.empty()){
			IProtocolFactory *factory = factories[0];
			if((factory->flags() & PROTOCOL_SENDS_FIRST)
					|| (T::flags & PROTOCOL_SENDS_FIRST)){
				LOG_ERROR("Service::add_protocol: protocols `%s` and `%s` cannot use the same port %d",
					factory->name(), T::name, port);
				return false;
			}
		}

		factories.push_back(new ProtocolFactory<T>);
		return true;
	}

	bool open(void){
		if(socket != nullptr){
			LOG_ERROR("Service::open: service already open");
			return false;
		}

		socket = net_server_socket(port);
		if(socket == nullptr){
			LOG_ERROR("Service::open: failed to open service on port %d", port);
			return false;
		}

		// start accept chain
		auto callback = [this](Socket *sock, int error, int transfered)
			{ this->on_accept(sock, error, transfered); };
		if(!socket_async_accept(socket, callback)){
			LOG_ERROR("Service::open: failed to start accept chain on port %d", port);
			socket_close(socket);
			return false;
		}
		return true;
	}

	void close(void){
		if(socket != nullptr){
			socket_close(socket);
			socket = nullptr;
		}
	}

	void on_accept(Socket *sock, int error, int transfered){
		if(error == 0){
			// accept new connection
			//

			// chain next accept
			auto callback = [this](Socket *sock, int error, int transfered)
				{ this->on_accept(sock, error, transfered); };
			if(socket_async_accept(socket, callback))
				return;
		}

		// socket error
		LOG_ERROR("Service::on_accept: socket error! trying to re-open service");
		close(); open();
	}

	Protocol *make_protocol(Connection *conn, uint32 identifier){
		for(IProtocolFactory *factory : factories){
			if(factory->identifier() == identifier)
				return factory->make_protocol(conn);
		}
		return nullptr;
	}
};

/*************************************

	Server Implementation

*************************************/
static std::vector<Service*> services;
static bool running = false;

template<typename T>
bool server_add_protocol(int port){
	if(running){
		LOG_ERROR("server_add_protocol: server already running");
		return false;
	}

	auto it = std::find_if(services.begin(), services.end(), [port](Service *service)
		{ return (service->get_port() == port); });

	Service *service;
	if(it == services.end()){
		service = new Service(port);
		services.push_back(service);
	} else {
		service = *it;
	}

	return service->add_protocol<T>();
}

void server_run(void){
	// open services
	for(Service *service : services)
		service->open();

	while(running && net_work() == 0)
		continue;

	// close services
	for(Service *service : services)
		service->close();
}

void server_stop(void){
	running = false;
}

Protocol *server_make_protocol(Service *service,
		Connection *conn, uint32 identifier){
	return service->make_protocol(conn, identifier);
}
