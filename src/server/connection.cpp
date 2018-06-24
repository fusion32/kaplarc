#include "connection.h"
#include "message.h"
#include "outputmessage.h"
#include "protocol.h"
#include "server.h"
#include "../log.h"
#include "asio.h"

#include <mutex>
#include <queue>

/*************************************

	Connection

*************************************/
// connection flags
#define CONNECTION_CLOSED		0x03 // (0x01 | CONNECTION_SHUTDOWN)
#define CONNECTION_SHUTDOWN		0x02
#define CONNECTION_FIRST_MSG		0x04

// connection settings
#define CONNECTION_TIMEOUT		10000

class Connection{
public:
	// deleted operations
	Connection(void) = delete;
	Connection(const Connection&) = delete;
	Connection &operator=(const Connection&) = delete;

	// connection control
	asio::ip::tcp::socket		*socket;
	Service				*service;
	std::shared_ptr<Protocol>	protocol;
	uint32				flags;
	uint32				rdwr_count;
	asio::steady_timer		timeout;
	std::recursive_mutex		mtx;

	// connection messages
	Message				input;
	std::queue<OutputMessage>	output_queue;

	// constructor/destructor
	Connection(asio::ip::tcp::socket *socket_, Service *service_)
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

static void connmgr_internal_close(
	const std::shared_ptr<Connection> &conn, bool forced = false);

static void timeout_handler(std::weak_ptr<Connection> wconn,
		const asio::error_code &ec){
	auto conn = wconn.lock();
	if(conn){
		std::lock_guard<std::recursive_mutex> lock(conn->mtx);
		if(conn->rdwr_count == 0){
			connmgr_internal_close(conn, true);
		}else{
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
	msg->length = msg->get_u16() + 2;
	if(ec || (conn->flags & CONNECTION_SHUTDOWN) != 0 ||
			transfered == 0 || msg->length == 0 ||
			(msg->length + 2) > msg->capacity){
		connmgr_internal_close(conn);
		return;
	}

	// chain body read
	asio::async_read(*conn->socket, asio::buffer(msg->buffer+2, msg->length-2),
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

	// create protocol if the connection doesn't have it yet
	if(conn->protocol == nullptr){
		conn->protocol = service_make_protocol(conn->service, conn, msg);
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

	// release written message
	conn->rdwr_count++;
	conn->output_queue.pop();

	// chain next message write if output_queue is not empty
	if(!conn->output_queue.empty()){
		Message *next = conn->output_queue.front().get();
		asio::async_write(*conn->socket, asio::buffer(next->buffer, next->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_write(std::move(conn), ec, transfered); });
	}
}

/*************************************

	Connection Manager

*************************************/
void connmgr_accept(void *socket_ptr, Service *service){
	auto socket = (asio::ip::tcp::socket*)socket_ptr;
	auto conn = std::make_shared<Connection>(socket, service);
	auto wconn = std::weak_ptr<Connection>(conn);

	// initialize connection
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(service_has_single_protocol(service)){
		conn->protocol = service_make_protocol(service, conn);
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
	asio::async_read(*socket, asio::buffer(conn->input.buffer, 2),
		[conn](const asio::error_code &ec, std::size_t transfered)
			{ on_read_length(std::move(conn), ec, transfered); });
}

static void connmgr_internal_close(const std::shared_ptr<Connection> &conn, bool forced){
	// the goal here is to shutdown the socket, interrupting the
	// read chain while keeping it open to send any pending output
	// messages and when these are done, the connection will be
	// properly released by it's destructor
	std::error_code ec;
	if(forced && conn->socket != nullptr){
		conn->socket->cancel(ec);
		DEBUG_CHECK(!ec, "socket cancel: %s", ec.message().c_str());
	}
	if((conn->flags & CONNECTION_SHUTDOWN) == 0){
		// shutdown connection
		conn->flags |= CONNECTION_SHUTDOWN;
		if(conn->socket != nullptr && conn->socket->is_open()){
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

void connmgr_send(const std::shared_ptr<Connection> &conn, OutputMessage msg){
	std::lock_guard<std::recursive_mutex> lock(conn->mtx);
	if(conn->flags & CONNECTION_SHUTDOWN)
		return;
	if(conn->output_queue.size() == 0){
		// start write chain
		asio::async_write(*conn->socket, asio::buffer(msg->buffer, msg->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_write(std::move(conn), ec, transfered); });
	}
	conn->output_queue.push(std::move(msg));
}
