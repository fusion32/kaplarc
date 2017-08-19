
#include <algorithm>
#include <vector>
#include "../log.h"
#include "connection.h"
#include "../netlib/network.h"
#include "server.h"
#include "protocol.h"


/*************************************

	Service Implementation

*************************************/

bool Service::open(void){
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
	if(!socket_async_accept(socket, on_accept, this)){
		LOG_ERROR("Service::open: failed to start accept chain on port %d", port);
		socket_close(socket);
		return false;
	}
	return true;
}

void Service::close(void){
	if(socket != nullptr){
		socket_close(socket);
		socket = nullptr;
	}
}

void Service::on_accept(Socket *sock, int error, int transfered, void *udata){
	Service *service = (Service*)udata;
	if(error == 0){
		// accept new connection
		ConnMgr::instance().accept(sock, service);

		// chain next accept
		if(socket_async_accept(service->socket, on_accept, udata))
			return;
	}

	// socket error
	LOG_ERROR("Service::on_accept: socket error! trying to re-open service");
	service->close();
	service->open();
}

Service::Service(int port_)
	: port(port_), socket(nullptr){}

Service::~Service(void){
	if(socket != nullptr){
		socket_close(socket);
		socket = nullptr;
	}
	for(IProtocolFactory *factory : factories)
		delete factory;
}

int Service::get_port(void) const{
	return port;
}

bool Service::single_protocol(void) const{
	if(factories.empty())
		return false;
	return factories[0]->single();
}

template<typename T>
bool Service::add_protocol(void){
	if(!factories.empty()){
		IProtocolFactory *factory = factories[0];
		if((factory->flags() & PROTOCOL_SINGLE)
				|| (T::flags & PROTOCOL_SINGLE)){
			LOG_ERROR("Service::add_protocol: protocols `%s` and `%s` cannot use the same port %d",
				factory->name(), T::name, port);
			return false;
		}
	}

	factories.push_back(new ProtocolFactory<T>);
	return true;
}

Protocol *Service::make_protocol(Connection *conn, uint32 identifier){
	for(IProtocolFactory *factory : factories){
		if(identifier == PROTOCOL_ANY
			|| factory->identifier() == identifier)
			return factory->make_protocol(conn);
	}
	return nullptr;
}

/*************************************

	Server Implementation

*************************************/

Server::Server(void)
  : running(false) {}

Server::~Server(void){}

template<typename T>
bool Server::add_protocol(int port){
	if(running){
		LOG_ERROR("server_add_protocol: server already running");
		return false;
	}

	auto it = std::find_if(services.begin(), services.end(),
		[port](Service *service) {
			return (service->port == port);
		});

	Service *service;
	if(it == services.end()){
		service = new Service(port);
		services.push_back(service);
	} else {
		service = *it;
	}

	return service->add_protocol<T>();
}

void Server::run(void){
	// open services
	for(Service *service : services)
		service->open();

	while(running && net_work() == 0)
		continue;

	// close services
	for(Service *service : services)
		service->close();
}

void Server::stop(void){
	running = false;
}

Protocol *Server::make_protocol(Service *service,
		Connection *conn, uint32 identifier){
	return service->make_protocol(conn, identifier);
}
