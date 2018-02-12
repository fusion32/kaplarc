#include "connection.h"
#include "protocol.h"
#include "server.h"

/*************************************

	Connection

*************************************/
Connection::Connection(asio::ip::tcp::socket *socket_, Service *service_)
  :	socket(socket_),
	service(service_),
	protocol(nullptr),
	flags(0),
	rdwr_count(0),
	timeout(socket->get_io_service()) {
}

Connection::~Connection(void){
	if(socket != nullptr){
		socket->close();
		delete socket;
		socket = nullptr;
	}
	LOG("connection released");
}

/*************************************

	Connection Callbacks

*************************************/
static void timeout_handler(std::weak_ptr<Connection> wconn,
	const asio::error_code &ec);
static void on_read_length(std::shared_ptr<Connection> conn,
	const asio::error_code &ec, std::size_t transfered);
static void on_read_body(std::shared_ptr<Connection> conn,
	const asio::error_code &ec, std::size_t transfered);
static void on_write(std::shared_ptr<Connection> conn,
	const asio::error_code &ec, std::size_t transfered);

// special helper function to avoid deadlocking
static void connmgr_internal_close(const std::shared_ptr<Connection> &conn);

static void timeout_handler(std::weak_ptr<Connection> wconn,
		const asio::error_code &ec){

	LOG("use count = %d", wconn.use_count());
	auto conn = wconn.lock();
	if(conn){
		std::lock_guard<std::mutex> lock(conn->mtx);
		if(conn->rdwr_count == 0){
			connmgr_internal_close(conn);
		}else{
			LOG("conn->rdwr_count = %lu", conn->rdwr_count);
			conn->rdwr_count = 0;
			conn->timeout.expires_from_now(
				std::chrono::milliseconds(CONNECTION_TIMEOUT));
			conn->timeout.async_wait(
				[wconn](const asio::error_code &ec)
					{ timeout_handler(std::move(wconn), ec); });
		}
	}
}

static void on_read_length(std::shared_ptr<Connection> conn,
		const asio::error_code &ec, std::size_t transfered){

	Message *msg = &conn->input;
	std::lock_guard<std::mutex> lock(conn->mtx);
	msg->readpos = 0;
	msg->length = msg->get_u16();
	if(!ec && transfered > 0 && msg->length > 0
			&& (msg->length + 2) < msg->capacity
			&& (conn->flags & CONNECTION_SHUTDOWN) == 0){

		// chain body read
		asio::async_read(*conn->socket, asio::buffer(msg->buffer, msg->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_read_body(std::move(conn), ec, transfered); });
		return;
	}

	connmgr_internal_close(conn);
}

static void on_read_body(std::shared_ptr<Connection> conn,
		const asio::error_code &ec, std::size_t transfered){

	Message *msg = &conn->input;
	std::lock_guard<std::mutex> lock(conn->mtx);
	if(!ec && transfered > 0
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
			asio::async_read(*conn->socket, asio::buffer(msg->buffer, 2),
				[conn](const asio::error_code &ec, std::size_t transfered)
					{ on_read_length(std::move(conn), ec, transfered); });
			return;
		}
	}

	connmgr_internal_close(conn);
}

static void on_write(std::shared_ptr<Connection> conn,
		const asio::error_code &ec, std::size_t transfered){

	std::lock_guard<std::mutex> lock(conn->mtx);
	if(!ec && transfered > 0){
		conn->rdwr_count++;
		output_pool_release(conn->output_queue.front());
		conn->output_queue.pop();
		if(!conn->output_queue.empty()){
			Message *next = conn->output_queue.front();
			asio::async_write(*conn->socket, asio::buffer(next->buffer, next->length),
				[conn](const asio::error_code &ec, std::size_t transfered)
					{ on_write(std::move(conn), ec, transfered); });
		}
	}else{
		connmgr_internal_close(conn);
	}
}

/*************************************

	Connection Manager

*************************************/
static std::vector<std::shared_ptr<Connection>> connections;
static std::mutex mtx;

void connmgr_accept(asio::ip::tcp::socket *socket, Service *service){
	auto conn = std::make_shared<Connection>(socket, service);

	// initialize connection
	{	std::lock_guard<std::mutex> lock(conn->mtx);
		if(service_has_single_protocol(conn->service)){
			conn->protocol = service_make_protocol(conn->service, conn);
			conn->protocol->on_connect();
		}


		// run timeout timer
		auto wconn = std::weak_ptr<Connection>(conn);
		conn->rdwr_count = 0;
		conn->timeout.expires_from_now(
			std::chrono::milliseconds(CONNECTION_TIMEOUT));
		conn->timeout.async_wait(
			[wconn](const asio::error_code &ec)
				{ timeout_handler(std::move(wconn), ec); });

		// start read chain
		asio::async_read(*conn->socket, asio::buffer(conn->input.buffer, 2),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_read_length(std::move(conn), ec, transfered); });
	}

	// insert connection into list
	{	std::lock_guard<std::mutex> lock(mtx);
		connections.push_back(conn);
	}
}

static void connmgr_internal_close(const std::shared_ptr<Connection> &conn){
	// the goal here is to shutdown the socket, interrupting the
	// read chain while keeping it open to send any pending output
	// messages and also remove the connection manager shared_ptr
	// reference so when there are no more output messages, the
	// connection is properly released by it's destructor

	if((conn->flags & CONNECTION_SHUTDOWN) == 0){
		// remove connection reference from connection list
		{	std::lock_guard<std::mutex> lock(mtx);
			auto it = std::find(connections.cbegin(), connections.cend(), conn);
			if(it != connections.end())
				connections.erase(it);
		}

		// shutdown connection
		conn->flags |= CONNECTION_SHUTDOWN;
		conn->socket->shutdown(asio::ip::tcp::socket::shutdown_receive);
		if(conn->protocol)
			conn->protocol->on_close();
	}
}

void connmgr_close(const std::shared_ptr<Connection> &conn){
	std::lock_guard<std::mutex> lock(conn->mtx);
	connmgr_internal_close(conn);
}

void connmgr_send(const std::shared_ptr<Connection> &conn, Message *msg){
	std::lock_guard<std::mutex> lock(conn->mtx);
	if(conn->flags & CONNECTION_SHUTDOWN)
		return;

	conn->output_queue.push(msg);
	if(conn->output_queue.size() == 1){
		// start write chain
		asio::async_write(*conn->socket, asio::buffer(msg->buffer, msg->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_write(std::move(conn), ec, transfered); });
	}
}
