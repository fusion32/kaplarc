#include "iocp.h"
#include "../log.h"
#include "../system.h"

#ifdef PLATFORM_WINDOWS

struct iocp_ctx server_ctx = {
	.iocp = NULL,
	.AcceptEx = NULL,
	.GetAcceptExSockAddrs = NULL,
	.initialized = false,
};
static struct iocp_ctx *ctx = &server_ctx;

static bool server_create_iocp(void){
	if(ctx->iocp != NULL) return true;
	ctx->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(ctx->iocp == NULL){
		LOG_ERROR("server_create_iocp: failed to create"
			" completion port (error = %d)", GetLastError());
		return false;
	}
	return true;
}

static void server_close_iocp(void){
	if(ctx->iocp == NULL) return;
	CloseHandle(ctx->iocp);
	ctx->iocp = NULL;
}

static bool server_load_winsock_extensions(void){
	DWORD dummy;
	SOCKET s;
	GUID guid0 = WSAID_ACCEPTEX;
	GUID guid1 = WSAID_GETACCEPTEXSOCKADDRS;
	int ret;

	// create dummy socket needed for WSAIoctl
	s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	if(s == INVALID_SOCKET){
		LOG_ERROR("wsa_init: failed to create dummy"
			" socket (error = %d)", WSAGetLastError());
		return false;
	}

	// load AcceptEx
	ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid0, sizeof(GUID), &ctx->AcceptEx,
		sizeof(LPFN_ACCEPTEX), &dummy, NULL, NULL);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("wsa_load_extensions: failed to load"
		  " 'AcceptEx' (error = %d)", WSAGetLastError());
		closesocket(s);
		return false;
	}

	// load GetAcceptExSockAddrs
	ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid1, sizeof(GUID), &ctx->GetAcceptExSockAddrs,
		sizeof(LPFN_GETACCEPTEXSOCKADDRS), &dummy, NULL, NULL);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("wsa_load_extensions: failed to load"
		  " 'GetAcceptExSockAddrs' (error = %d)", WSAGetLastError());
		closesocket(s);
		return false;
	}

	closesocket(s);
	return true;
}

static bool server_init_winsock(void){
	WSADATA wsd;
	int ret;
	ret = WSAStartup(MAKEWORD(2, 2), &wsd);
	if(ret != 0){
		LOG_ERROR("wsa_startup: WSAStartup"
			" failed (error = %d)", ret);
		return false;
	}

	if(!server_load_winsock_extensions()){
		WSACleanup();
		return false;
	}
	return true;
}

static void server_cleanup_winsock(void){
	WSACleanup();
}

bool server_internal_init(void){
	if(ctx->initialized)
		return true;
	if(!server_init_winsock())
		goto fail0;
	if(!server_create_iocp())
		goto fail1;
	if(!connmgr_init())
		goto fail2;
	if(!svcmgr_init())
		goto fail3;
	ctx->initialized = true;
	return true;

	// the server won't get to run if this fails
	// so we can safely release everything without
	// running `internal_server_work`
fail3:	connmgr_shutdown();
fail2:	server_close_iocp();
fail1:	server_cleanup_winsock();
fail0:	return false;
}

void server_internal_shutdown(void){
	if(!ctx->initialized) return;
	// server shouldn't be running when this is called
	// so it's safe to release everything
	svcmgr_shutdown();
	connmgr_shutdown();
	server_close_iocp();
	server_cleanup_winsock();
}

#define MAX_EVENTS 256
#define TIMEOUT_INTERVAL 5000 // 5s
void server_internal_work(void){
	// timeout
	static int64 next_timeout_check = 0;
	int64 now;

	// events
	OVERLAPPED_ENTRY evs[MAX_EVENTS];
	struct async_ov *ov;
	DWORD error, transferred, flags;
	ULONG ev_count, i;
	BOOL ret;
	bool interrupt = false;

	while(!interrupt){
		// timeout check
		now = sys_tick_count();
		if(next_timeout_check <= now){
			connmgr_timeout_check();
			next_timeout_check = now + TIMEOUT_INTERVAL;
		}
		// event processing
		ret = GetQueuedCompletionStatusEx(ctx->iocp, evs, MAX_EVENTS,
			&ev_count, (DWORD)(next_timeout_check - now), FALSE);
		if(!ret) break;
		for(i = 0; i < ev_count; i += 1){
			ov = (struct async_ov*)evs[i].lpOverlapped;
			if(ov == NULL){
				interrupt = true;
				continue;
			}
			ret = WSAGetOverlappedResult(ov->s, &ov->ov,
				&transferred, FALSE, &flags);
			if(ret)	error = NOERROR;
			else	error = WSAGetLastError();
			ov->complete(ov->data, error, transferred);
		}
	}
}

void server_internal_interrupt(void){
	PostQueuedCompletionStatus(ctx->iocp, 0, 0, NULL);
}

#endif //PLATFORM_WINDOWS
