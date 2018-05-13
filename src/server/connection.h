// NOTE: the unforunate use of a recursive_mutex here is due to the
// protocol handlers being called inside the critical section (as doing
// otherwise, would require an input message pool). This allows the use
// of connmgr_send or connmgr_close from within these handlers.

#ifndef SERVER_CONNECTION_H_
#define SERVER_CONNECTION_H_

#include "../def.h"
#include "../message.h"
#include "../scheduler.h"
#include "asio.h"
#include "outputmessage.h"
#include "protocol.h"

#include <mutex>
#include <memory>
#include <vector>
#include <queue>

// connection flags
#define CONNECTION_CLOSED		0x03 // (0x01 | CONNECTION_SHUTDOWN)
#define CONNECTION_SHUTDOWN		0x02
#define CONNECTION_FIRST_MSG		0x04

// connection settings
#define CONNECTION_TIMEOUT		10000

class Service;
class Connection{
// this is a low-level class so making all attributes public will make
// this interface more readable and easier to implement
public:
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
void connmgr_send(const std::shared_ptr<Connection> &conn, OutputMessage msg);

#endif //CONNECTION_H_
