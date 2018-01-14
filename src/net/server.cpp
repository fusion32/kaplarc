
#include <algorithm>
#include <vector>
#include "../log.h"
#include "../netlib/network.h"
#include "connection.h"
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
	auto callback = [this](Socket *sock, int error, int transfered)
		{ Service::on_accept(sock, error, transfered, this); };
	if(!socket_async_accept(socket, callback)){
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

void Service::on_accept(Socket *sock, int error, int transfered, Service *service){
	if(error == 0){
		// accept new connection
		ConnMgr::instance()->accept(sock, service);

		// chain next accept
		auto callback = [service](Socket *sock, int error, int transfered)
			{ Service::on_accept(sock, error, transfered, service); };
		if(socket_async_accept(service->socket, callback))
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

Protocol *Service::make_protocol(const std::shared_ptr<Connection> &conn){
	if(factories.empty())
		return nullptr;
	return factories[0]->make_protocol(conn);
}

Protocol *Service::make_protocol(const std::shared_ptr<Connection> &conn, uint32 identifier){
	for(IProtocolFactory *factory : factories){
		if(factory->identifier() == identifier)
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

void Server::run(void){
	// open services
	for(Service *service : services)
		service->open();

	running = true;
	while(running && net_work() == 0)
		continue;

	// close services
	for(Service *service : services)
		service->close();
}

void Server::stop(void){
	running = false;
}
