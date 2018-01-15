
#include <algorithm>
#include <vector>
#include "../log.h"
#include "../netlib/network.h"
#include "connection.h"
#include "server.h"
#include "protocol.h"


/*************************************

	Service Class

*************************************/

class Service{
public:
	// service control
	std::vector<IProtocolFactory*>	factories;
	Socket				*socket;
	int				port;

	// constructor/destructor
	Service(int port);
	~Service(void);

	// delete operations
	Service(void) = delete;
	Service(const Service&) = delete;
	Service(Service&&) = delete;
	Service &operator=(const Service&) = delete;
	Service &operator=(Service&&) = delete;
};

Service::Service(int port_)
 : port(port_), socket(nullptr) {}

Service::~Service(void){
	if(socket != nullptr){
		socket_close(socket);
		socket = nullptr;
	}

	for(IProtocolFactory *factory : factories)
		delete factory;
	factories.clear();
}

/*************************************

	Service Static Helpers

*************************************/
static bool service_open(Service *service);
static void service_close(Service *service);
static bool service_add_factory(Service *service, IProtocolFactory *factory);
static void service_on_accept(Socket *sock, int error,
			int transfered, Service *service);

static bool service_open(Service *service){
	if(service->socket != nullptr){
		LOG_ERROR("Service::open: service already open");
		return false;
	}

	service->socket = net_server_socket(service->port);
	if(service->socket == nullptr){
		LOG_ERROR("Service::open: failed to open service on port %d", service->port);
		return false;
	}

	// start accept chain
	auto callback = [service](Socket *sock, int error, int transfered)
		{ service_on_accept(sock, error, transfered, service); };
	if(!socket_async_accept(service->socket, callback)){
		LOG_ERROR("Service::open: failed to start accept chain on port %d", service->port);
		socket_close(service->socket);
		return false;
	}
	return true;
}

static void service_close(Service *service){
	if(service->socket != nullptr){
		socket_close(service->socket);
		service->socket = nullptr;
	}
}

static bool service_add_factory(Service *service, IProtocolFactory *f){
	if(!service->factories.empty()){
		IProtocolFactory *factory = service->factories[0];
		if(factory->single() || f->single()){
			LOG_ERROR("service_add_factory: protocols `%s` and `%s` cannot use the same port (%d)",
				factory->name(), f->name(), service->port);
			return false;
		}
	}
	service->factories.push_back(f);
	return true;
}

static void service_on_accept(Socket *sock, int error,
			int transfered, Service *service){

	if(error == 0){
		// accept new connection
		connmgr_accept(sock, service);

		// chain next accept
		auto callback = [service](Socket *sock, int error, int transfered)
			{ service_on_accept(sock, error, transfered, service); };
		if(socket_async_accept(service->socket, callback))
			return;
	}

	// socket error
	LOG_ERROR("Service::on_accept: socket error! trying to re-open service");
	service_close(service);
	service_open(service);
}

/*************************************

	Service Public Interface

*************************************/
int service_port(Service *service){
	return service->port;
}

bool service_has_single_protocol(Service *service){
	if(service->factories.empty())
		return false;
	return service->factories[0]->single();
}

Protocol *service_make_protocol(Service *service,
		const std::shared_ptr<Connection> &conn){
	if(service->factories.empty())
		return nullptr;
	return service->factories[0]->make_protocol(conn);
}

Protocol *service_make_protocol(Service *service,
		const std::shared_ptr<Connection> &conn, uint32 identifier){
	for(IProtocolFactory *factory : service->factories){
		if(factory->identifier() == identifier)
			return factory->make_protocol(conn);
	}
	return nullptr;
}


/*************************************

	Server Public Interface

*************************************/
static std::vector<Service*> services;
static bool running = false;

void server_run(void){
	for(Service *service : services)
		service_open(service);

	running = true;
	while(running && net_work() == 0)
		continue;

	for(Service *service : services)
		service_close(service);
}

void server_stop(void){
	running = false;
}

bool server_add_factory(int port, IProtocolFactory *factory){
	if(running){
		LOG_ERROR("server_add_protocol_factory: server already running");
		return false;
	}

	Service *service;
	auto it = std::find_if(services.begin(), services.end(),
		[port](Service *service){ return service->port == port; });
	if(it == services.end()){
		service = new Service(port);
		services.push_back(service);
	}else{
		service = *it;
	}

	return service_add_factory(service, factory);
}
