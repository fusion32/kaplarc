// NOTE: the unforunate use of a recursive_mutex here is due to the
// protocol handlers being called inside the critical section (as doing
// otherwise, would require an input message pool). This allows the use
// of connmgr_send or connmgr_close from within these handlers.

#ifndef SERVER_CONNECTION_H_
#define SERVER_CONNECTION_H_

#include "../def.h"
#include "outputmessage.h"

class Service;
class Connection;
// putting void here to avoid including asio from a header
void connection_accept(void *socket_ptr, Service *service);
void connection_close(Connection *conn);
void connection_send(Connection *conn, OutputMessage msg);
void connection_incref(Connection *conn);
void connection_decref(Connection *conn);

#endif //CONNECTION_H_
