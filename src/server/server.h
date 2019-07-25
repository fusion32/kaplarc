#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "../def.h"
#include "protocol.h"

struct Connection;
struct Message;
struct OutputMessage;

// Server interface
void server_run(void);
void server_stop(void);
bool server_add_protocol(Protocol *protocol, int port);

// Connection interface
void connection_close(Connection *conn);
void connection_send(Connection *conn, OutputMessage *msg);
void connection_incref(Connection *conn);
void connection_decref(Connection *conn);

#endif //SERVER_H_
