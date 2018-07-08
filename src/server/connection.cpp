#include "connection.h"
#include "message.h"
#include "outputmessage.h"
#include "protocol.h"
#include "server.h"
#include "../log.h"
#include "../shared.h"
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

class Connection
  : public Shared<Connection> {
public:
	// deleted operations
	Connection(void) = delete;
	Connection(const Connection&) = delete;
	Connection &operator=(const Connection&) = delete;

	// connection control
	asio::ip::tcp::socket	*socket;
	Service			*service;
	Protocol		*protocol;
	uint32			flags;
	uint32			rdwr_count;
	asio::steady_timer	timeout;
	std::recursive_mutex	mtx;

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

 Connection Lock
(used to avoid releasing reference
while inside the critical section)

*************************************/
class ConnectionLock{
private:
	Connection *conn_;
	bool decref_on_exit_;

public:
	ConnectionLock(Connection *conn)
	  : conn_(conn), decref_on_exit_(false) {
		conn_->mtx.lock();
	}
	~ConnectionLock(void){
		conn_->mtx.unlock();
		if(decref_on_exit_)
			conn_->decref();
	}

	void decref_on_exit(void){
		decref_on_exit_ = true;
	}
};

/*************************************

 Connection Internals

*************************************/
static void conn_internal_close(Connection *conn, bool forced = false);
static void timeout_handler(Connection *conn, const asio::error_code &ec);
static void on_read_length(Connection *conn, const asio::error_code &ec,
		std::size_t transfered);
static void on_read_body(Connection *conn, const asio::error_code &ec,
		std::size_t transfered);
static void on_write(Connection *conn, const asio::error_code &ec,
		std::size_t transfered);

static void timeout_handler(Connection *conn, const asio::error_code &ec){
	ConnectionLock lock(conn);
	// ec can only be asio::error::operation_aborted
	if(ec || conn->rdwr_count == 0){
		conn_internal_close(conn, true);
		lock.decref_on_exit();
	}else{
		conn->rdwr_count = 0;
		conn->timeout.expires_from_now(
			std::chrono::milliseconds(CONNECTION_TIMEOUT));
		conn->timeout.async_wait([conn](const asio::error_code &ec)
				{ timeout_handler(conn, ec); });
	}
}

static void on_read_length(Connection *conn, const asio::error_code &ec,
		std::size_t transfered){
	Message *msg = &conn->input;
	ConnectionLock lock(conn);
	msg->readpos = 0;
	msg->length = msg->get_u16() + 2;
	if(ec || transfered == 0 ||
	  (conn->flags & CONNECTION_SHUTDOWN) != 0 ||
	  msg->length == 0 || msg->length > msg->capacity){
		conn_internal_close(conn,
			ec == asio::error::connection_reset ||
			ec == asio::error::connection_aborted);
		lock.decref_on_exit();
		return;
	}

	// chain body read
	asio::async_read(*conn->socket, asio::buffer(msg->buffer+2, msg->length-2),
		[conn](const asio::error_code &ec, std::size_t transfered)
			{ on_read_body(conn, ec, transfered); });
}

static void on_read_body(Connection *conn, const asio::error_code &ec,
		std::size_t transfered){
	Message *msg = &conn->input;
	ConnectionLock lock(conn);
	if(ec || transfered == 0 ||
	  (conn->flags & CONNECTION_SHUTDOWN) != 0){
		conn_internal_close(conn,
			ec == asio::error::connection_reset ||
			ec == asio::error::connection_aborted);
		lock.decref_on_exit();
		return;
	}

	// create protocol
	if(conn->protocol == nullptr){
		conn->protocol = service_make_protocol(conn->service, conn, msg);
		if(conn->protocol == nullptr){
			conn_internal_close(conn);
			lock.decref_on_exit();
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
			{ on_read_length(conn, ec, transfered); });
}

static void on_write(Connection *conn, const asio::error_code &ec,
		std::size_t transfered){
	ConnectionLock lock(conn);
	if(ec || transfered == 0){
		conn_internal_close(conn,
			ec == asio::error::connection_reset ||
			ec == asio::error::connection_aborted);
		lock.decref_on_exit();
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
				{ on_write(conn, ec, transfered); });
	}else{
		// else release connection reference from the write handler
		lock.decref_on_exit();
	}
}

/*************************************

 Connection Public Interface

*************************************/
void connection_accept(void *socket_ptr, Service *service){
	auto *socket = (asio::ip::tcp::socket*)socket_ptr;
	auto *conn = new Connection(socket, service);

	// initialize connection
	ConnectionLock lock(conn);
	if(service_has_single_protocol(service)){
		// connection already has a reference which
		// will belong to the protocol
		conn->protocol = service_make_protocol(service, conn);
		conn->protocol->on_connect();
	}

	// setup timeout timer
	conn->incref();
	conn->rdwr_count = 0;
	conn->timeout.expires_from_now(
		std::chrono::milliseconds(CONNECTION_TIMEOUT));
	conn->timeout.async_wait(
		[conn](const asio::error_code &ec)
			{ timeout_handler(conn, ec); });

	// start read chain
	conn->incref();
	asio::async_read(*socket, asio::buffer(conn->input.buffer, 2),
		[conn](const asio::error_code &ec, std::size_t transfered)
			{ on_read_length(conn, ec, transfered); });
}

static void conn_internal_close(Connection *conn, bool forced){
	// the goal here is to shutdown the socket, interrupting the
	// read chain while keeping it open to send any pending output
	// messages and when these are done, the connection will be
	// properly released by it's destructor
	asio::error_code ec;
	if(forced && conn->socket != nullptr){
		conn->socket->cancel(ec);
		DEBUG_CHECK(!ec, "socket cancel: %s", ec.message().c_str());
		conn->timeout.cancel(ec);
		DEBUG_CHECK(!ec, "timeout cancel: %s", ec.message().c_str());
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
			conn->protocol->decref();
			conn->protocol = nullptr;
		}
	}
}

void connection_close(Connection *conn){
	ConnectionLock lock(conn);
	conn_internal_close(conn);
}

void connection_send(Connection *conn, OutputMessage msg){
	ConnectionLock lock(conn);
	if(conn->flags & CONNECTION_SHUTDOWN)
		return;
	if(conn->output_queue.size() == 0){
		// start write chain
		conn->incref();
		asio::async_write(*conn->socket, asio::buffer(msg->buffer, msg->length),
			[conn](const asio::error_code &ec, std::size_t transfered)
				{ on_write(conn, ec, transfered); });
	}
	conn->output_queue.push(std::move(msg));
}

void connection_incref(Connection *conn){
	conn->incref();
}

void connection_decref(Connection *conn){
	conn->decref();
}
