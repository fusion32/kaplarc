#ifndef KAPLAR_SERVER_SERVER_H_
#define KAPLAR_SERVER_SERVER_H_ 1

#include "../common.h"
#include "protocol.h"

// server interface
bool server_init(void);
void server_shutdown(void);
void server_sync(void (*fp)(void*), void *arg);
bool svcmgr_add_protocol(struct protocol *protocol, int port);

// connection interface
void connection_close(uint32 uid);
void connection_abort(uint32 uid);
// `connection_userdata` will not fail while the connection is alive
// so it can't fail when used inside the protocol callbacks but might
// if used elsewhere returning NULL in that case
void **connection_userdata(uint32 uid);
bool connection_send(uint32 uid, uint8 *data, uint32 datalen);

#endif //KAPLAR_SERVER_SERVER_H_
