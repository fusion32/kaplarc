#include "iocp.h"
#include "../log.h"

#ifdef PLATFORM_WINDOWS

/* Service Structure */
#define MAX_SERVICE_PROTOCOLS 4
struct service{
	SOCKET s;
	bool closing;
	int pending_work;
	int port;
	int num_protocols;
	struct protocol protocols[MAX_SERVICE_PROTOCOLS];
	struct async_ov accept_ov;
	SOCKET accept_socket;
	uint8 accept_buffer[(sizeof(struct sockaddr_in) + 16) * 2];
};

/* Service Manager Data */
#define MAX_SERVER_SERVICES 4
static int num_services;
static struct service services[MAX_SERVER_SERVICES];
static struct iocp_ctx *ctx = &server_ctx;

/* STATIC FWD DECL */
static SOCKET internal_create_iocp_socket(void);
static void service_on_accept(void *data, DWORD err, DWORD transferred);
static bool service_start_async_accept(struct service *svc);
static void service_async_accept(struct service *svc);
static bool service_open(struct service *svc);
static void internal_service_close(struct service *svc);
static void service_start_closing(struct service *svc);

/* IMPL START */
static SOCKET internal_create_iocp_socket(void){
	// @TODO: using the socket as the COMPLETION_KEY when binding
	// the socket to the iocp will make the COMPLETION_KEY from
	// the OVERLAPPED_ENTRY be the socket and save us some (minimal)
	// memory from adding it to every async_ov
	DEBUG_ASSERT(ctx->iocp != NULL);
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if(s == INVALID_SOCKET)
		return INVALID_SOCKET;
	if(CreateIoCompletionPort((HANDLE)s,
	  ctx->iocp, 0, 0) != ctx->iocp){
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;
}

static void service_on_accept(void *data, DWORD err, DWORD transferred){
	struct service *svc = data;
	struct sockaddr_in *local_addr, *remote_addr;
	int local_addr_len, remote_addr_len;
	SOCKET s = svc->accept_socket;

	// decrease pending work
	svc->pending_work -= 1;

	// handle service closing
	if(svc->closing){
		closesocket(s); // close new socket
		// close service if this was the last pending work
		if(svc->pending_work == 0)
			internal_service_close(svc);
		return;
	}

	// handle errors
	if(err != NOERROR){
		closesocket(s); // close new socket
		service_async_accept(svc); // try to chain next accept
		return;
	}

	// get new connection address
	ctx->GetAcceptExSockAddrs(svc->accept_buffer, 0,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		(struct sockaddr**)&local_addr, &local_addr_len,
		(struct sockaddr**)&remote_addr, &remote_addr_len);
	DEBUG_ASSERT(local_addr_len == sizeof(struct sockaddr_in));
	DEBUG_ASSERT(remote_addr_len == sizeof(struct sockaddr_in));

	// start new connection
	connmgr_start_connection(s, remote_addr, svc);

	// chain next accept
	service_async_accept(svc);
}

static bool service_start_async_accept(struct service *svc){
	SOCKET s;
	DWORD error, transferred;
	BOOL ret;

	s = internal_create_iocp_socket();
	if(s == INVALID_SOCKET){
		LOG_ERROR("service_start_async_accept:"
			" failed to create socket");
		return false;
	}
	memset(&svc->accept_ov.ov, 0, sizeof(OVERLAPPED));
	svc->accept_ov.s = svc->s;
	svc->accept_ov.complete = service_on_accept;
	svc->accept_ov.data = svc;
	svc->accept_socket = s;
	ret = ctx->AcceptEx(svc->s, s, svc->accept_buffer, 0,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		&transferred, &svc->accept_ov.ov);
	if(!ret){
		error = WSAGetLastError();
		if(error != WSA_IO_PENDING){
			LOG_ERROR("service_start_async_accept:"
				"AcceptEx failed (error = %d)", error);
			closesocket(s);
			return false;
		}
	}
	svc->pending_work += 1;
	return true;
}

#define MAX_TRIES_BEFORE_REOPEN 5
static void service_async_accept(struct service *svc){
	DEBUG_ASSERT(svc->s != INVALID_SOCKET);
	for(int i = 0; i < MAX_TRIES_BEFORE_REOPEN; i += 1){
		if(service_start_async_accept(svc))
			return;
		LOG_WARNING("service_async_accept: failed to"
			"start accept operation (try #%d)", i+1);
	}
	LOG_ERROR("service_async_accept: failed to start accept"
		" operation after %d tries", MAX_TRIES_BEFORE_REOPEN);

	// try to reopen service if there is no pending work
	if(svc->pending_work != 0)
		return;
	LOG_ERROR("service_async_accept: trying to reopen service...");
	internal_service_close(svc);
	if(!service_open(svc)){
		LOG_ERROR("service_async_accept: reopen failed");
		LOG_ERROR("service_async_accept: service is now out");
	}
}

static bool service_open(struct service *svc){
	SOCKET s;
	struct sockaddr_in addr;
	if(svc->s != INVALID_SOCKET){
		LOG_WARNING("service_open: service is already open");
		return true;
	}

	s = internal_create_iocp_socket();
	if(s == INVALID_SOCKET){
		LOG_ERROR("service_open: failed to create socket");
		return false;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(svc->port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(s, (struct sockaddr*)&addr,
	  sizeof(struct sockaddr_in)) == SOCKET_ERROR){
		LOG_ERROR("service_open: failed to bind service"
			" to port %d (error = %d)",
			svc->port, WSAGetLastError());
		closesocket(s);
		return false;
	}
	if(listen(s, SOMAXCONN) == SOCKET_ERROR){
		LOG_ERROR("service_open: failed to listen"
			" (error = %d)", WSAGetLastError());
		closesocket(s);
		return false;
	}
	svc->s = s;
	if(!service_start_async_accept(svc)){
		LOG_ERROR("service_open: failed to start"
			" accept operation");
		svc->s = INVALID_SOCKET;
		closesocket(s);
		return false;
	}
	return true;
}

static void internal_service_close(struct service *svc){
	DEBUG_ASSERT(svc != NULL);
	DEBUG_ASSERT(svc->s != INVALID_SOCKET);
	closesocket(svc->s);
	svc->s = INVALID_SOCKET;
	svc->closing = false;
}

static void service_start_closing(struct service *svc){
	DEBUG_ASSERT(svc->s != INVALID_SOCKET);
	// if the service has no pending work, it's socket will be
	// closed here, else it'll be closed when all pending work
	// has been processed
	if(svc->pending_work == 0){
		internal_service_close(svc);
	}else{
		svc->closing = true;
		CancelIoEx((HANDLE)svc->s, NULL);
	}
}

bool svcmgr_init(void){
	int i;
	for(i = 0; i < num_services; i += 1){
		if(!service_open(&services[i]))
			goto fail;
	}
	return true;

	// shutdown initialized services
fail:	for(i -= 1; i >= 0; i -= 1)
		internal_service_close(&services[i]);
	return false;
}

void svcmgr_shutdown(void){
	for(int i = 0; i < num_services; i += 1)
		internal_service_close(&services[i]);
}

bool svcmgr_add_protocol(struct protocol *protocol, int port){
	int i;
	struct service *svc = NULL;
	DEBUG_ASSERT(!ctx->initialized && protocol != NULL);
	for(i = 0; i < num_services; i += 1){
		if(services[i].port == port){
			svc = &services[i];
			break;
		}
	}
	if(svc == NULL){
		if(num_services >= MAX_SERVER_SERVICES)
			return false;
		svc = &services[num_services++];
		svc->s = INVALID_SOCKET;
		svc->closing = false;
		svc->pending_work = 0;
		svc->port = port;
		svc->num_protocols = 1;
		memcpy(&svc->protocols[0],
			protocol, sizeof(struct protocol));
	}else{
		if(protocol->sends_first ||
		  svc->num_protocols >= MAX_SERVICE_PROTOCOLS ||
		  svc->protocols[0].sends_first)
			return false;
		memcpy(&svc->protocols[svc->num_protocols++],
			protocol, sizeof(struct protocol));
	}
	return true;
}

bool service_sends_first(struct service *svc){
	DEBUG_ASSERT(svc->num_protocols > 0);
	return svc->protocols[0].sends_first;
}

struct protocol *service_first_protocol(struct service *svc){
	DEBUG_ASSERT(svc->num_protocols > 0);
	return &svc->protocols[0];
}
struct protocol *service_select_protocol(struct service *svc,
		uint8 *data, uint32 datalen){
	for(int i = 0; i < svc->num_protocols; i += 1){
		if(svc->protocols[i].identify(data, datalen))
			return &svc->protocols[i];
	}
	return NULL;
}

#endif //PLATFORM_WINDOWS
