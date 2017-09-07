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

	ConnMgr Helpers

*************************************/
void ConnMgr::begin(const std::shared_ptr<Connection> &conn){
	if(conn->flags & (CONNECTION_CLOSED | CONNECTION_CLOSING))
		return;

	std::lock_guard<std::mutex> lguard(conn->mtx);
	if(conn->service->single_protocol()){
		//conn->protocol = conn->service->make_protocol(conn);
		conn->protocol->on_connect();
	}

	auto wconn = std::weak_ptr<Connection>(conn);
	conn->rd_timeout = scheduler_add(CONNECTION_RD_TIMEOUT,
		[conn=wconn](void){ ConnMgr::read_timeout_handler(conn); });

	if(conn->rd_timeout != SCHREF_INVALID){
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ ConnMgr::on_read_length(sock, error, transfered, conn); };
		if(socket_async_read(conn->socket, (char*)conn->input.buffer, 2, callback))
			return;

		cancel_rd_timeout(conn);
	}

	// close connection if the read chain is not properly started
	ConnMgr::instance()->close(conn);
}

void ConnMgr::cancel_rd_timeout(const std::shared_ptr<Connection> &conn){
	if(!scheduler_remove(conn->rd_timeout))
		conn->flags |= CONNECTION_RD_TIMEOUT_CANCEL;
}

void ConnMgr::cancel_wr_timeout(const std::shared_ptr<Connection> &conn){
	if(!scheduler_remove(conn->wr_timeout))
		conn->flags |= CONNECTION_WR_TIMEOUT_CANCEL;
}

/*************************************

	Connection Callbacks

*************************************/
void ConnMgr::read_timeout_handler(const std::weak_ptr<Connection> &conn){
}

void ConnMgr::write_timeout_handler(const std::weak_ptr<Connection> &conn){
}

void ConnMgr::on_read_length(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn){
}

void ConnMgr::on_read_body(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn){
}

void ConnMgr::on_write(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn){
}

/*************************************

	Connection Manager

*************************************/
ConnMgr::ConnMgr(void) {}
ConnMgr::~ConnMgr(void) {}

void ConnMgr::accept(Socket *socket, Service *service){
	// initialize connection
	auto conn = std::make_shared<Connection>(socket, service);
	begin(conn);

	// insert connection into list
	std::lock_guard<std::mutex> lguard(mtx);
	connections.push_back(conn);
}

void ConnMgr::close(const std::shared_ptr<Connection> &conn){
}
