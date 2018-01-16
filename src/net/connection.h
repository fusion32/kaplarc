#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "../def.h"
#include "../scheduler.h"
#include "../netlib/network.h"
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
#define CONNECTION_MAX_OUTPUT	8

class Service;
class Protocol;
class Connection{
// this is a low-level class so making all attributes public will make
// this interface more readable and easier to implement
public:
	// connection control
	Socket			*socket;
	Service			*service;
	Protocol		*protocol;
	uint32			flags;
	uint32			rdwr_count;
	SchRef			timeout;
	std::recursive_mutex	mtx;

	// connection messages
	Message			input;
	Message			output[CONNECTION_MAX_OUTPUT];
	std::queue<Message*>	output_queue;

	// constructor/destructor
	Connection(Socket *socket_, Service *service_);
	~Connection(void);

	// delete operations
	Connection(void) = delete;
	Connection(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection &operator=(const Connection&) = delete;
	Connection &operator=(Connection&&) = delete;
};

void connmgr_accept(Socket *socket, Service *service);
void connmgr_close(const std::shared_ptr<Connection> &conn);
void connmgr_send(const std::shared_ptr<Connection> &conn, Message *msg);
Message *connmgr_get_output_message(const std::shared_ptr<Connection> &conn);

#endif //CONNECTION_H_
