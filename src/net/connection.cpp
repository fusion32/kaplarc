#include "connection.h"
#include "protocol.h"
#include "server.h"


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

/*************************************

	Connection Callbacks

*************************************/
static void timeout_handler(const std::weak_ptr<Connection> &wconn);
static void on_read_length(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn);
static void on_read_body(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn);
static void on_write(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn);

static void timeout_handler(const std::weak_ptr<Connection> &wconn){
	auto conn = wconn.lock();
	if(conn){
		std::lock_guard<std::recursive_mutex> lock(conn->mtx);
		if(conn->rdwr_count == 0){
			connmgr_close(conn);
		}else{
			conn->rdwr_count = 0;
			conn->timeout = scheduler_add(CONNECTION_TIMEOUT,
				[wconn](void){ timeout_handler(wconn); });
		}
	}
}

static void on_read_length(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn){

	Message *msg = &conn->input;
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	msg->readpos = 0;
	msg->length = msg->get_u16();
	if(error == 0 && transfered > 0 && msg->length > 0
			&& (msg->length+2) < MESSAGE_BUFFER_LEN
			&& (conn->flags & CONNECTION_SHUTDOWN) == 0){
		// chain body read
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ on_read_body(sock, error, transfered, conn); };
		if(socket_async_read(sock, (char*)(msg->buffer + 2),
				msg->length, callback))
			return;
	}

	connmgr_close(conn);
}

static void on_read_body(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn){

	Message *msg = &conn->input;
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(error == 0 && transfered > 0
			&& (conn->flags & CONNECTION_SHUTDOWN) == 0){

		// uint32 checksum = adler32(msg->buffer + 6, msg->length - 4);
		// if(checksum == msg->peek_u32())
		//	msg->readpos += 4;

		if(conn->protocol == nullptr)
			conn->protocol = service_make_protocol(
				conn->service, conn, msg->get_byte());

		if(conn->protocol != nullptr){
			if((conn->flags & CONNECTION_FIRST_MSG) == 0){
				conn->flags |= CONNECTION_FIRST_MSG;
				conn->protocol->on_recv_first_message(msg);
			}else{
				conn->protocol->on_recv_message(msg);
			}

			// chain next message read
			conn->rdwr_count++;
			auto callback = [conn](Socket *sock, int error, int transfered)
				{ on_read_length(sock, error, transfered, conn); };
			if(socket_async_read(sock, (char*)msg->buffer, 2, callback))
				return;
		}
	}

	connmgr_close(conn);
}

static void on_write(Socket *sock, int error, int transfered,
			const std::shared_ptr<Connection> &conn){

	if(error != 0 || transfered <= 0){
		connmgr_close(conn);
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	conn->rdwr_count++;
	conn->output_queue.front()->release();
	conn->output_queue.pop();
	if(!conn->output_queue.empty()){
		Message *next = conn->output_queue.front();
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ on_write(sock, error, transfered, conn); };
		if(!socket_async_write(sock, (char*)next->buffer, next->length, callback))
			connmgr_close(conn);
	}

}

/*************************************

	Connection Manager

*************************************/
static std::vector<std::shared_ptr<Connection>> connections;
static std::mutex mtx;

void connmgr_accept(Socket *socket, Service *service){
	auto conn = std::make_shared<Connection>(socket, service);

	// initialize connection
	{	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
		if(service_has_single_protocol(conn->service)){
			conn->protocol = service_make_protocol(conn->service, conn);
			conn->protocol->on_connect();
		}

		auto wconn = std::weak_ptr<Connection>(conn);
		conn->rdwr_count = 0;
		conn->timeout = scheduler_add(CONNECTION_TIMEOUT,
			[wconn](void){ timeout_handler(wconn); });
		if(conn->timeout == SCHREF_INVALID)
			return;

		auto callback = [conn](Socket *sock, int error, int transfered)
			{ on_read_length(sock, error, transfered, conn); };
		if(!socket_async_read(conn->socket, (char*)conn->input.buffer, 2, callback)){
			scheduler_remove(conn->timeout);
			return;
		}
	}

	// insert connection into list
	{	std::lock_guard<std::mutex> lock(mtx);
		connections.push_back(conn);
	}
}

void connmgr_close(const std::shared_ptr<Connection> &conn){
	// the goal here is to shutdown the socket, interrupting the
	// read chain while keeping it open to send any pending output
	// messages
	// also remove this shared_ptr reference so when there are no
	// more output messages, the connection is properly released
	// by the it's destructor

	// remove connection from connection list
	{	std::lock_guard<std::mutex> lock(mtx);
		auto it = std::find(connections.cbegin(), connections.cend(), conn);
		if(it != connections.end())
			connections.erase(it);
	}

	// shutdown connection
	{	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
		if((conn->flags & CONNECTION_SHUTDOWN) == 0){
			conn->flags |= CONNECTION_SHUTDOWN;
			socket_shutdown(conn->socket, SOCKET_SHUT_RD);
		}
	}
}

void connmgr_send(const std::shared_ptr<Connection> &conn, Message *msg){
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	conn->output_queue.push(msg);
	if(conn->output_queue.size() == 1){
		auto callback = [conn](Socket *sock, int error, int transfered)
			{ on_write(sock, error, transfered, conn); };
		if(!socket_async_write(conn->socket, (char*)msg->buffer, msg->length, callback))
			connmgr_close(conn);
	}
}

Message *connmgr_get_output_message(const std::shared_ptr<Connection> &conn){
	for(int i = 0; i < CONNECTION_MAX_OUTPUT; ++i){
		if(conn->output[i].try_acquire())
			return &conn->output[i];
	}
	return nullptr;
}
