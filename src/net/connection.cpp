#include "connection.h"


/*************************************

	Connection

*************************************/
Connection::Connection(Socket *socket_, Service *service_)
  :	socket(socket_),
	service(service_),
	protocol(nullptr),
	flags(0),
	rdwr_count(0),
	timeout(SCHREF_INVALID) {

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
}


/*************************************

	ConnMgr Helpers

*************************************/
void ConnMgr::begin(const std::shared_ptr<Connection> &conn){
	std::lock_guard<std::mutex> lock(conn->mtx);
	if(conn->flags & CONNECTION_SHUTDOWN)
		return;

	if(conn->service->single_protocol()){
		conn->protocol = conn->service->make_protocol(conn);
		conn->protocol->on_connect();
	}

	auto wconn = std::weak_ptr<Connection>(conn);
	conn->rdwr_count = 0;
	conn->timeout = scheduler_add(CONNECTION_TIMEOUT,
		[wconn](void){ ConnMgr::timeout_handler(wconn); });

	if(conn->timeout != SCHREF_INVALID){
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ ConnMgr::on_read_length(sock, error, transfered, conn); };
		if(socket_async_read(conn->socket, (char*)conn->input.buffer, 2, callback))
			return;

		scheduler_remove(conn->timeout);
	}

	// close connection if the read chain is not properly started
	ConnMgr::instance()->close(conn);
}

/*************************************

	Connection Callbacks

*************************************/
void ConnMgr::timeout_handler(const std::weak_ptr<Connection> &wconn){
	auto conn = wconn.lock();
	if(conn){
		std::lock_guard<std::mutex> lock(conn->mtx);
		if(conn->rdwr_count == 0){
			ConnMgr::instance()->close(conn);
		}else{
			conn->rdwr_count = 0;
			conn->timeout = scheduler_add(CONNECTION_TIMEOUT,
				[wconn](void){ ConnMgr::timeout_handler(wconn); });
		}
	}
}

void ConnMgr::on_read_length(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn){

	Message *msg = &conn->input;

	std::lock_guard<std::mutex> lock(conn->mtx);
	msg->readpos = 0;
	msg->length = msg->get_u16();
	if(error == 0 && transfered > 0 && msg->length > 0
			&& (msg->length+2) < MESSAGE_BUFFER_LEN
			&& (conn->flags & CONNECTION_SHUTDOWN) == 0){
		// chain body read
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ ConnMgr::on_read_body(sock, error, transfered, conn); };
		if(socket_async_read(conn->socket, (char*)(msg->buffer + 2),
				msg->length, callback))
			return;
	}

	ConnMgr::instance()->close(conn);
}

void ConnMgr::on_read_body(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn){

	Message *msg = &conn->input;

	std::lock_guard<std::mutex> lock(conn->mtx);
	if(error == 0 && transfered > 0
			&& (conn->flags & CONNECTION_SHUTDOWN) == 0){

		// checksum
		// on receive message

		// chain next message read
		conn->rdwr_count++;
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ ConnMgr::on_read_length(sock, error, transfered, conn); };
		if(socket_async_read(conn->socket, (char*)msg->buffer, 2, callback))
			return;
	}
	ConnMgr::instance()->close(conn);
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
	std::lock_guard<std::mutex> lock(mtx);
	connections.push_back(conn);
}

void ConnMgr::close(const std::shared_ptr<Connection> &conn){
}
