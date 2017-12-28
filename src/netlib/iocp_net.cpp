#ifdef _WIN32
#include "iocp_private.h"
#include "../log.h"

static WSAData	wsa_data;
static HANDLE	iocp;

LPFN_ACCEPTEX			pfnAcceptEx = nullptr;
LPFN_GETACCEPTEXSOCKADDRS	pfnGetAcceptExSockaddrs = nullptr;

static bool load_extensions(void){
	int ret;
	DWORD dummy;
	SOCKET fd;
	GUID guid0 = WSAID_ACCEPTEX;
	GUID guid1 = WSAID_GETACCEPTEXSOCKADDRS;

	// create dummy socket
	if((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET){
		LOG_ERROR("load_extensions: failed to create dummy socket (error = %d)", GetLastError());
		return false;
	}

	// load AcceptEx
	ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid0, sizeof(GUID), &pfnAcceptEx, sizeof(LPFN_ACCEPTEX),
		&dummy, nullptr, nullptr);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("load_extensions: failed to retrieve `AcceptEx` extension (error = %d)", GetLastError());
		closesocket(fd);
		return false;
	}

	// load GetAcceptExSockaddrs
	ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid1, sizeof(GUID), &pfnGetAcceptExSockaddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
		&dummy, nullptr, nullptr);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("load_extensions: failed to retrieve `GetAcceptExSockaddrs` extension (error = %d)", GetLastError());
		closesocket(fd);
		return false;
	}

	closesocket(fd);
	return true;
}

bool net_init(void){
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if(ret != 0){
		LOG_ERROR("net_init: WSAStartup failed (error = %d)", ret);
		return false;
	}

	if(LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2){
		LOG_ERROR("net_init: Winsock version error");
		return false;
	}

	if(!load_extensions()){
		LOG_ERROR("net_init: failed to load windows sockets extensions");
		return false;
	}

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if(iocp == nullptr){
		LOG_ERROR("net_init: failed to create i/o completion port (error = %d)", GetLastError());
		return false;
	}

	return true;
}

void net_shutdown(void){
	if(iocp != nullptr){
		CloseHandle(iocp);
		iocp = nullptr;
	}

	WSACleanup();
}

Socket *net_socket(void){
	SOCKET fd;
	Socket *sock;

	fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if(fd == INVALID_SOCKET){
		LOG_ERROR("net_socket: failed to create socket (error = %d)", GetLastError());
		return nullptr;
	}

	if(CreateIoCompletionPort((HANDLE)fd, iocp, 0, 0) != iocp){
		LOG_ERROR("net_socket: failed to register socket to io completion port (error = %d)", GetLastError);
		closesocket(fd);
		return nullptr;
	}

	sock = new Socket;
	sock->fd = fd;
	sock->local_addr = nullptr;
	sock->remote_addr = nullptr;
	for(int i = 0; i < MAX_OPS; ++i)
		sock->ops[i].opcode.store(OP_NONE, std::memory_order_release);
	return sock;
}

Socket *net_server_socket(int port){
	sockaddr_in addr;
	Socket *sock;

	sock = net_socket();
	if(sock == nullptr){
		LOG_ERROR("net_server_socket: failed to create socket");
		return nullptr;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if(bind(sock->fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR){
		socket_close(sock);
		LOG_ERROR("net_server_socket: failed to bind socket to port `%d` (error = %d)", port, GetLastError());
		return nullptr;
	}

	if(listen(sock->fd, SOMAXCONN) == SOCKET_ERROR){
		socket_close(sock);
		LOG_ERROR("net_server_socket: failed to listen on port `%d` (error = %d)", port, GetLastError());
		return nullptr;
	}
	return sock;
}

int net_work(void){
	SocketOp *op;
	DWORD transfered, error;
	ULONG_PTR completion_key;
	int dummy;
	BOOL ret;

	ret = GetQueuedCompletionStatus(iocp, &transfered,
		&completion_key, (OVERLAPPED**)&op, NET_WORK_TIMEOUT);

	error = NO_ERROR;
	if(ret == FALSE)
		error = GetLastError();

	if(op == nullptr)
		return posix_error(error);

	if(op->opcode == OP_ACCEPT){
		pfnGetAcceptExSockaddrs(op->socket->addr_buffer, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&op->socket->local_addr, &dummy,
			(sockaddr**)&op->socket->remote_addr, &dummy);
	}

	op->complete(op->socket, posix_error(error), transfered);
	// release std::function resources
	op->complete = nullptr;
	op->opcode.store(OP_NONE, std::memory_order_relaxed);
	return 0;
}
#endif //_WIN32
