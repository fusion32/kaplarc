#include "connection.h"
#include "protocol.h"
#include "server.h"
#include "../log.h"

/*************************************

	Connection

*************************************/
Connection::Connection(asio::ip::tcp::socket *socket_, Service *service_)
  :	socket(socket_),
	service(service_),
	protocol(nullptr),
	flags(0),
	rdwr_count(0),
	timeout(socket_->get_io_service()) {
}

Connection::~Connection(void){
	if(protocol != nullptr){
		protocol->on_close();
		protocol = nullptr;
	}

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

static void connmgr_internal_close(const std::shared_ptr<Connection> &conn);

static void timeout_handler(std::weak_ptr<Connection> wconn,
		const asio::error_code &ec){

	LOG("use count = %d", wconn.use_count());
	auto conn = wconn.lock();
	if(conn){
		std::lock_guard<std::recursive_mutex> lock(conn->mtx);
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
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	msg->readpos = 0;
	msg->length = msg->get_u16();
	if(ec || (conn->flags & CONNECTION_SHUTDOWN) != 0 ||
			transfered == 0 || msg->length == 0 ||
			(msg->length + 2) > msg->capacity){
		connmgr_internal_close(conn);
		return;
	}

	// chain body read
	asio::async_read(*conn->socket, asio::buffer(msg->buffer+2, msg->length),
		[conn](const asio::error_code &ec, std::size_t transfered)
			{ on_read_body(std::move(conn), ec, transfered); });
}

static void on_read_body(std::shared_ptr<Connection> conn,
		const asio::error_code &ec, std::size_t transfered){
	Message *msg = &conn->input;
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(ec || transfered == 0 || (conn->flags & CONNECTION_SHUTDOWN) != 0){
		connmgr_internal_close(conn);
		return;
	}

	//uint32 checksum = adler32(msg->buffer + 6, msg->length - 4);
	//if(checksum == msg->peek_u32())
	//	msg->readpos += 4;

	// create protocol if the connection doesn't have it yet
	if(conn->protocol == nullptr){
		conn->protocol = service_make_protocol(
			conn->service, conn, msg->get_byte());
		if(conn->protocol == nullptr){
			connmgr_internal_close(conn);
			return;
		}
	}

	// dispatch message to protocol
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
}

static void on_write(std::shared_ptr<Connection> conn,
		const asio::error_code &ec, std::size_t transfered){
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(ec || transfered == 0){
		connmgr_internal_close(conn);
		return;
	}

	// release written message back to the pool
	conn->rdwr_count++;
	output_pool_release(conn->output_queue.front());
	conn->output_queue.pop();

	// chain next message write if output_queue is not empty
	if(!conn->output_queue.empty()){
		Message *next = conn->output_queue.front();
		asio::async_write(*conn->socket, asio::buffer(next->buffer, next->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_write(std::move(conn), ec, transfered); });
	}
}

/*************************************

	Connection Manager

*************************************/

void connmgr_accept(asio::ip::tcp::socket *socket, Service *service){
	auto conn = std::make_shared<Connection>(socket, service);
	auto wconn = std::weak_ptr<Connection>(conn);

	// initialize connection
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(service_has_single_protocol(conn->service)){
		conn->protocol = service_make_protocol(conn->service, conn);
		conn->protocol->on_connect();
	}


	// setup timeout timer
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

static void connmgr_internal_close(const std::shared_ptr<Connection> &conn){
	// the goal here is to shutdown the socket, interrupting the
	// read chain while keeping it open to send any pending output
	// messages and when these are done, the connection will be
	// properly released by it's destructor
	if((conn->flags & CONNECTION_SHUTDOWN) == 0){
		// shutdown connection
		conn->flags |= CONNECTION_SHUTDOWN;
		if(conn->socket != nullptr && conn->socket->is_open()){
			std::error_code ec;
			conn->socket->shutdown(asio::ip::tcp::socket::shutdown_receive, ec);
			DEBUG_CHECK(!ec, "socket shutdown: %s", ec.message().c_str());
		}

		// release protocol
		if(conn->protocol){
			conn->protocol->on_close();
			conn->protocol = nullptr;
		}
	}
}

void connmgr_close(const std::shared_ptr<Connection> &conn){
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	connmgr_internal_close(conn);
}

void connmgr_send(const std::shared_ptr<Connection> &conn, Message *msg){
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
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
