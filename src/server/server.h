#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "../def.h"
#include "protocol.h"

struct connection;
struct packed_data;

extern struct protocol protocol_echo;

// Server interface
bool server_init(void);
void server_shutdown(void);
int server_work(void);
bool svcmgr_add_protocol(struct protocol *protocol, int port);

// Connection interface
void connection_close(struct connection *c);
void connection_abort(struct connection *c);
bool connection_send(struct connection *c, uint8 *data, uint32 datalen);

#endif //SERVER_H_
