#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "../def.h"
#include "protocol.h"

struct connection;
struct packed_data;

extern struct protocol *protocol_test;

// Server interface
bool server_init(void);
void server_shutdown(void);
int server_work(void);
bool svcmgr_add_protocol(struct protocol *protocol, int port);

// Connection interface
void connection_close(struct connection *conn);
void connection_send(struct connection *conn, struct packed_data *msg);
void connection_incref(struct connection *conn);
void connection_decref(struct connection *conn);

#endif //SERVER_H_
