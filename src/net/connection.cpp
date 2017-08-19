#include "connection.h"


/*************************************

	Connection

*************************************/
Connection::Connection(Socket *socket_, Service *service_)
  :	socket(socket_),
	service(service_),
	protocol(nullptr),
	flags(0),
	rd_timeout(SCHREF_INVALID),
	wr_timeout(SCHREF_INVALID) {

	input.release();
	for(int i = 0; i < CONNECTION_MAX_OUTPUT; ++i)
		output[i].release();
}

Connection::~Connection(void){
	if(socket != nullptr){
		socket_close(socket);
		socket = nullptr;
	}
}

Message *Connection::get_output_message(void){
	for(int i = 0; i < CONNECTION_MAX_OUTPUT; ++i){
		if(output[i].try_acquire())
			return &output[i];
	}
	return nullptr;
}

void Connection::send(Message *msg){
	// check if connection is closed/closing
	if(flags & (CONNECTION_CLOSED | CONNECTION_CLOSING))
		return;

	//
}


/*************************************

	Connection Callbacks

*************************************/
void ConnMgr::begin(Connection *conn){
	if(conn->flags & (CONNECTION_CLOSED | CONNECTION_CLOSING))
		return;

	std::lock_guard<std::mutex> lguard(conn->mtx);
	if(conn->service->single_protocol()){
		conn->protocol = conn->service->make_protocol(conn, PROTOCOL_ANY);
		conn->protocol->on_connect();
	}

	conn->incref();
	conn->rd_timeout = scheduler_add(CONNECTION_RD_TIMEOUT,
		[conn](void){ ConnMgr::read_timeout_handler(conn); });
	if(conn->rd_timeout != SCHREF_INVALID){
		conn->incref();
		if(socket_async_read(conn->socket, (char*)conn->input.buffer, 2,
				ConnMgr::on_read_length, conn))
			return;
	}
}

void ConnMgr::read_timeout_handler(Connection *conn){
}

void ConnMgr::write_timeout_handler(Connection *conn){
}

void ConnMgr::on_read_length(Socket *sock, int error, int transfered, void *udata){
	Connection *conn = (Connection*)udata;
}

void ConnMgr::on_read_body(Socket *sock, int error, int transfered, void *udata){
	Connection *conn = (Connection*)udata;
}

void ConnMgr::on_write(Socket *sock, int error, int transfered, void *udata){
	Connection *conn = (Connection*)udata;
}

/*************************************

	Connection Manager

*************************************/
ConnMgr::ConnMgr(void) {}
ConnMgr::~ConnMgr(void) {}

void ConnMgr::accept(Socket *socket, Service *service){
	// initialize connection
	Connection *conn = new Connection(socket, service);
	begin(conn);

	// insert connection into list
	std::lock_guard<std::mutex> lguard(mtx);
	connections.push_back(conn);
}

void ConnMgr::close(Connection *conn){
}