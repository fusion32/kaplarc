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
private:
	// connection control
	Socket			*socket;
	Service			*service;
	Protocol		*protocol;
	uint32			flags;
	uint32			rdwr_count;
	SchRef			timeout;
	std::mutex		mtx;

	// connection messages
	Message			input;
	Message			output[CONNECTION_MAX_OUTPUT];
	std::queue<Message*>	output_queue;

	// befriend the Connection Manager
	friend class ConnMgr;

public:
	Connection(Socket *socket_, Service *service_);
	~Connection(void);
};

class ConnMgr{
private:
	// connection list
	std::vector<std::shared_ptr<Connection>> connections;
	std::mutex mtx;

	// delete copy and move operations
	ConnMgr(const ConnMgr&) = delete;
	ConnMgr(ConnMgr&&) = delete;
	ConnMgr &operator=(const ConnMgr&) = delete;
	ConnMgr &operator=(ConnMgr&&) = delete;

	// private construtor and destrutor
	ConnMgr(void);
	~ConnMgr(void);

	// connection callbacks
	static void timeout_handler(const std::weak_ptr<Connection> &wconn);
	static void on_read_length(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn);
	static void on_read_body(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn);
	static void on_write(Socket *sock, int error, int transfered,
				const std::shared_ptr<Connection> &conn);

public:
	static ConnMgr *instance(void){
		static ConnMgr instance;
		return &instance;
	}
	void accept(Socket *socket, Service *service);
	void close(const std::shared_ptr<Connection> &conn);
	void send(const std::shared_ptr<Connection> &conn, Message *msg);
	Message *get_output_message(const std::shared_ptr<Connection> &conn);
};

#endif //CONNECTION_H_
