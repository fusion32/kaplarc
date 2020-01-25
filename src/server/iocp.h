#ifndef SERVER_IOCP_H_
#define SERVER_IOCP_H_ 1

#include "../def.h"

#ifdef PLATFORM_WINDOWS
#include "protocol.h"
#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <mswsock.h>

struct connection;
struct service;

struct async_ov{
	OVERLAPPED ov;
	SOCKET s;
	void (*complete)(void*, DWORD, DWORD);
	void *data;
};

struct iocp_ctx{
	HANDLE iocp;
	LPFN_ACCEPTEX AcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockAddrs;
	bool initialized;
};

// iocp_connmgr.c
bool connmgr_init(void);
void connmgr_start_shutdown(void);
void connmgr_shutdown(void);
void connmgr_start_connection(SOCKET s,
	struct sockaddr_in *addr,
	struct service *svc);
void connection_close(struct connection *c);
void connection_abort(struct connection *c);
bool connection_send(struct connection *c, uint8 *data, uint32 datalen);

// iocp_service.c
bool svcmgr_init(void);
void svcmgr_start_shutdown(void);
void svcmgr_shutdown(void);
bool svcmgr_add_protocol(struct protocol *proto, int port);
bool service_sends_first(struct service *svc);
struct protocol *service_first_protocol(struct service *svc);
struct protocol *service_select_protocol(struct service *svc,
		uint8 *data, uint32 datalen);

// iocp_server.c
extern struct iocp_ctx server_ctx;
bool server_init(void);
void server_shutdown(void);
int server_work(void);

#endif //PLATFORM_WINDOWS
#endif //SERVER_IOCP_H_
