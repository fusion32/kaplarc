#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "../def.h"
#include "protocol.h"

class Service;
class Connection;

class Message;
class OutputMessage;


// Service interface
int service_port(Service *service);
bool service_sends_first(Service *service);

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
