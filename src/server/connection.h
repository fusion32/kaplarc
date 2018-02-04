#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "../def.h"
#include "../scheduler.h"
#include "asio.h"
#include "message.h"

#include <mutex>
#include <memory>
#include <vector>
#include <queue>

// connection flags
#define CONNECTION_CLOSED		0x03 // (0x01 | CONNECTION_SHUTDOWN)
#define CONNECTION_SHUTDOWN		0x02
#define CONNECTION_FIRST_MSG		0x04

// connection settings
#define CONNECTION_TIMEOUT	10000

class Service;
class Protocol;
class Connection{
// this is a low-level class so making all attributes public will make
// this interface more readable and easier to implement
public:
	// connection control
	asio::ip::tcp::socket	*socket;
	Service			*service;
	Protocol		*protocol;
	uint32			flags;
	uint32			rdwr_count;
	asio::steady_timer	timeout;
	std::mutex		mtx;

	// connection messages
	Message			input;
	std::queue<Message*>	output_queue;

	// constructor/destructor
	Connection(asio::ip::tcp::socket *socket_, Service *service_);
	~Connection(void);

	// delete operations
	Connection(void) = delete;
	Connection(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection &operator=(const Connection&) = delete;
	Connection &operator=(Connection&&) = delete;
};

void connmgr_accept(asio::ip::tcp::socket *socket, Service *service);
void connmgr_close(const std::shared_ptr<Connection> &conn);
void connmgr_send(const std::shared_ptr<Connection> &conn, Message *msg);

#endif //CONNECTION_H_
