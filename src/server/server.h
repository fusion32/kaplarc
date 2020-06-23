#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include "../def.h"
#include "protocol.h"

// server interface
bool server_init(void);
void server_shutdown(void);
void server_sync(void (*fp)(void*), void *arg);
bool svcmgr_add_protocol(struct protocol *protocol, int port);

// connection interface
void connection_close(uint32 uid);
void connection_abort(uint32 uid);
bool connection_send(uint32 uid, uint8 *data, uint32 datalen);

#endif //SERVER_SERVER_H_
