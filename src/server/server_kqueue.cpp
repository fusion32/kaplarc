//#ifdef PLATFORM_BSD
#if 0

#include "../log.h"
#include "connection.h"
#include "server.h"
#include "protocol.h"
#include <vector>

#include "../ringbuffer.h"
#include "message.h"

#include <mutex>
#include <vector>

/*************************************

	Connection Class

*************************************/
#define CONNECTION_MAX_OUTPUT 16

class Connection
  : public Shared<Connection> {
public:
	// connection control
	Service		*service;
	Protocol	*protocol;
	int		socket;
	uint32		flags;
	uint32		rdwr_count;
	//asio::steady_timer	timeout;
	std::mutex	mtx;

	// connection messages
	Message		input;
	Message		output_pool[CONNECTION_MAX_OUTPUT];
	RingBuffer<Message*, CONNECTION_MAX_OUTPUT>
			output_queue;

	// constructor/destructor
	Connection(int socket_, Service *service_)
	  :	socket(socket_),
		service(service_),
		protocol(nullptr),
		flags(0),
		rdwr_count(0),
		timeout(socket_->get_io_service()){
	}
	~Connection(void){
		if(protocol != nullptr){
			protocol->on_close();
			protocol->decref();
			protocol = nullptr;
		}

		if(socket != nullptr){
			socket->close();
			delete socket;
			socket = nullptr;
		}
		LOG("connection released");
	}
};

/*************************************

	Service Class

*************************************/
class Service{
public:
	// service control
	Protocol	*protocols;
	int		socket;
	int		port;

	// constructor/destructor
	Service(int socket_, int port_)
	  : socket(socket_), port(port_) {}
	~Service(void){
		if(socket != -1)
			close(socket);
	}
};

/*************************************

	Service Static Helpers

*************************************/
static bool service_open(Service *service);
static void service_close(Service *service);
static bool service_add_protocol(Service *service, Protocol *protocol);
static void service_start_accept(Service *service);
static void service_on_accept(Service *service, int socket, int err);

static bool service_open(Service *service){
	if(service->socket != -1){
		LOG_ERROR("service_open: service has already been initialized\n");
		return false;
	}

	// initialize socket

	// start accept chain
	service_start_accept(service);
	return true;
}

static void service_close(Service *service){
	if(service->socket != -1){
		close(service->socket);
		service->socket = -1;
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
		connection_accept(socket, service);
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

bool service_sends_first(Service *service){
	if(service->protocols == nullptr)
		return false;
	return service->protocols->sends_first;
}


/*************************************

	Server Public Interface

*************************************/
static bool running = false;
static std::vector<Service*> services;

void server_run(void){
	if(running) return;
	for(Service *service : services)
		service_open(service);
	running = true;

	// run

	running = false;
	for(Service *service : services)
		service_close(service);
}

void server_stop(void){
	//
}

bool server_add_protocol(Protocol *protocol, int port){
}

#endif //PLATFORM_BSD
