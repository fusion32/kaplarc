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
void ConnMgr::read_timeout(Connection *conn){
}

void ConnMgr::write_timeout(Connection *conn){
}

void ConnMgr::on_read_length(Connection *conn, Socket *sock, int error, int transfered){
}

void ConnMgr::on_read_body(Connection *conn, Socket *sock, int error, int transfered){
}

void ConnMgr::on_write(Connection *conn, Socket *sock, int error, int transfered){
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