// NOTE: the unforunate use of a recursive_mutex here is due to the
// protocol handlers being called inside the critical section (as doing
// otherwise, would require an input message pool). This allows the use
// of connmgr_send or connmgr_close from within these handlers.

#ifndef SERVER_CONNECTION_H_
#define SERVER_CONNECTION_H_

#include "../def.h"
#include "outputmessage.h"
#include <memory>

class Service;
class Connection;
// putting void here to avoid including asio from a header
void connmgr_accept(void *socket_ptr, Service *service);
void connmgr_close(const std::shared_ptr<Connection> &conn);
void connmgr_send(const std::shared_ptr<Connection> &conn, OutputMessage msg);

#endif //CONNECTION_H_
