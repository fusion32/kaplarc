
#include <algorithm>
#include <vector>
#include "../log.h"
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
	asio::ip::tcp::acceptor		acceptor;
	int				port;

	// constructor/destructor
	Service(asio::io_service &ios_, int port_);
	~Service(void);

	// delete operations
	Service(void) = delete;
	Service(const Service&) = delete;
	Service(Service&&) = delete;
	Service &operator=(const Service&) = delete;
	Service &operator=(Service&&) = delete;
};

Service::Service(asio::io_service &ios_, int port_)
 : acceptor(ios_), port(port_) {}

Service::~Service(void){
	if(acceptor.is_open())
		acceptor.close();
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
static void service_start_accept(Service *service);
static void service_on_accept(asio::ip::tcp::socket *socket,
	Service *service, const asio::error_code &err);

static bool service_open(Service *service){
	if(service->acceptor.is_open()){
		LOG_ERROR("service_open: service already open");
		return false;
	}

	// initialize acceptor
	asio::ip::tcp::endpoint endpoint(asio::ip::address(), service->port);
	service->acceptor.open(endpoint.protocol());
	service->acceptor.set_option(
		asio::ip::tcp::acceptor::reuse_address(true));
	service->acceptor.bind(endpoint);
	service->acceptor.listen();

	// start accept chain
	service_start_accept(service);
	return true;
}

static void service_close(Service *service){
	if(service->acceptor.is_open())
		service->acceptor.close();
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

static void service_start_accept(Service *service){
	// start accept chain
	auto socket = new asio::ip::tcp::socket(
		service->acceptor.get_io_service());
	service->acceptor.async_accept(*socket,
		[socket, service](const asio::error_code &err) -> void
			{ service_on_accept(socket, service, err); });
}

static void service_on_accept(asio::ip::tcp::socket *socket,
	Service *service, const asio::error_code &err){

	if(!err){
		connmgr_accept(socket, service);
		// chain next accept
		service_start_accept(service);
	}else{
		// socket error
		LOG_ERROR("service_on_accept: socket error! trying to re-open service");
		delete socket;
		service_close(service);
		service_open(service);
	}
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
static asio::io_service io_service;
static std::vector<Service*> services;

void server_run(void){
	for(Service *service : services)
		service_open(service);

	//asio::io_service::work work(io_service);
	io_service.run();

	for(Service *service : services)
		service_close(service);
}

void server_stop(void){
	io_service.stop();
}

bool server_add_factory(int port, IProtocolFactory *factory){
	if(io_service.stopped()){
		LOG_ERROR("server_add_factory: server already running");
		return false;
	}

	Service *service;
	auto it = std::find_if(services.begin(), services.end(),
		[port](Service *service){ return service->port == port; });
	if(it == services.end()){
		service = new Service(io_service, port);
		services.push_back(service);
	}else{
		service = *it;
	}

	return service_add_factory(service, factory);
}

asio::io_service &server_io_service(void){
	return io_service;
}
