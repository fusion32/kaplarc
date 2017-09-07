#ifdef _WIN32

#include <atomic>
#include <errno.h>
#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <mswsock.h>

#include "../log.h"
#include "network.h"

#define OP_NONE		0x00
#define OP_ACCEPT	0x01
#define OP_READ		0x02
#define OP_WRITE	0x03
struct AsyncOp{
	OVERLAPPED		overlapped;
	std::atomic<int>	opcode;
	Socket			*socket;
	SocketCallback		complete;
};

#define SOCKET_MAX_OPS 8
struct Socket{
	SOCKET		fd;
	AsyncOp		ops[SOCKET_MAX_OPS];
	char		addr_buffer[sizeof(sockaddr_in) * 2 + 16];
	sockaddr_in	*local_addr;
	sockaddr_in	*remote_addr;
};

// windows handles
static WSAData	wsa_data;
static HANDLE	iocp;

// windows sockets extensions
static LPFN_ACCEPTEX			AcceptEx_;
static LPFN_GETACCEPTEXSOCKADDRS	GetAcceptExSockaddrs_;

static int posix_error(DWORD error){
	switch(error){
	case WSA_OPERATION_ABORTED:	return ECANCELED;
	case ERROR_NETNAME_DELETED:
	case ERROR_CONNECTION_ABORTED:
	case WSAECONNABORTED:		return ECONNABORTED;
	case WSAECONNRESET:		return ECONNRESET;
	case WSAENETRESET:		return ENETRESET;
	case WSAENETDOWN:		return ENETDOWN;
	case WSAENOTCONN:		return ENOTCONN;
	case WSAEWOULDBLOCK:		return EWOULDBLOCK;
	case WAIT_TIMEOUT:		return ETIMEDOUT;
	case NO_ERROR:			return 0;
	}

	// generic error
	return -1;
}

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
		&guid0, sizeof(GUID), &AcceptEx_, sizeof(LPFN_ACCEPTEX),
		&dummy, nullptr, nullptr);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("load_extensions: failed to retrieve `AcceptEx` extension (error = %d)", GetLastError());
		closesocket(fd);
		return false;
	}

	// load GetAcceptExSockaddrs
	ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid1, sizeof(GUID), &GetAcceptExSockaddrs_, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
		&dummy, nullptr, nullptr);
	if(ret == SOCKET_ERROR){
		LOG_ERROR("load_extensions: failed to retrieve `GetAcceptExSockaddrs` extension (error = %d)", GetLastError());
		closesocket(fd);
		return false;
	}

	closesocket(fd);
	return true;
}

/*************************************

	Network Management

*************************************/

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
		LOG_ERROR("net_init: failed to create io completion port (error = %d)", GetLastError());
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
	for(int i = 0; i < SOCKET_MAX_OPS; ++i)
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
	AsyncOp *op;
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
		GetAcceptExSockaddrs_(op->socket->addr_buffer, 0,
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


/*************************************

	Socket Management

*************************************/

static AsyncOp *socket_op(Socket *sock, int opcode){
	int cmp;
	for(int i = 0; i < SOCKET_MAX_OPS; ++i){
		cmp = OP_NONE;
		if(sock->ops[i].opcode.compare_exchange_strong(cmp, opcode,
				std::memory_order_acq_rel))
			return &sock->ops[i];
	}
	return nullptr;
}

uint32 socket_remote_address(Socket *sock){
	if(sock->remote_addr == nullptr) return 0;
	return sock->remote_addr->sin_addr.s_addr;
}

void socket_shutdown(Socket *sock, int how){
	int opcode;
	shutdown(sock->fd, how);
	for(int i = 0; i < SOCKET_MAX_OPS; ++i){
		opcode = sock->ops[i].opcode.load(std::memory_order_acquire);
		if((how == SOCKET_SHUT_RDWR && opcode != OP_NONE)
			|| (how == SOCKET_SHUT_RD && opcode == OP_READ)
			|| (how == SOCKET_SHUT_RD && opcode == OP_ACCEPT)
			|| (how == SOCKET_SHUT_WR && opcode == OP_WRITE))
			CancelIoEx((HANDLE)sock->fd, (OVERLAPPED*)&sock->ops[i]);

	}
}

void socket_close(Socket *sock){
	CancelIoEx((HANDLE)sock->fd, nullptr);
	closesocket(sock->fd);
	delete sock;
}

bool socket_async_accept(Socket *sock, SocketCallback cb){
	AsyncOp *op;
	BOOL ret;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_ACCEPT);
	if(op == nullptr){
		LOG_ERROR("socket_async_accept: maximum simultaneous operations reached (%d)", SOCKET_MAX_OPS);
		return false;
	}

	// initialize it
	memset(&op->overlapped, 0, sizeof(OVERLAPPED));
	op->complete = std::move(cb);
	op->socket = net_socket();
	if(op->socket == nullptr){
		op->opcode.store(OP_NONE, std::memory_order_relaxed);
		LOG_ERROR("socket_async_accept: failed to create new socket");
		return false;
	}

	// start accept operation
	ret = AcceptEx_(sock->fd, op->socket->fd, op->socket->addr_buffer, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		&transfered, (OVERLAPPED*)op);
	if(ret == FALSE){
		error = GetLastError();
		if(error != WSA_IO_PENDING){
			socket_close(op->socket);
			op->opcode.store(OP_NONE, std::memory_order_relaxed);
			LOG_ERROR("socket_async_accept: AcceptEx failed with error `%d`", error);
			return false;
		}
	}
	return true;
}

bool socket_async_read(Socket *sock, char *buf, int len, SocketCallback cb){
	AsyncOp *op;
	WSABUF data;
	DWORD flags;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_READ);
	if(op == nullptr){
		LOG_ERROR("socket_async_read: maximum simultaneous operations reached (%d)", SOCKET_MAX_OPS);
		return false;
	}

	// initialize it
	memset(&op->overlapped, 0, sizeof(OVERLAPPED));
	op->socket = sock;
	op->complete = std::move(cb);

	// start read operation
	flags = 0;
	data.len = len;
	data.buf = buf;
	if(WSARecv(sock->fd, &data, 1, &transfered, &flags,
			(OVERLAPPED*)op, nullptr) == SOCKET_ERROR){
		error = GetLastError();
		if(error != WSA_IO_PENDING){
			op->opcode.store(OP_NONE, std::memory_order_relaxed);
			LOG_ERROR("socket_async_read: WSARecv failed with error `%d`", error);
			return false;
		}
	}
	return true;
}

bool socket_async_write(Socket *sock, char *buf, int len, SocketCallback cb){
	AsyncOp *op;
	WSABUF data;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_WRITE);
	if(op == nullptr){
		LOG_ERROR("socket_async_write: maximum simultaneous operations reached (%d)", SOCKET_MAX_OPS);
		return false;
	}

	// initialize it
	memset(&op->overlapped, 0, sizeof(OVERLAPPED));
	op->socket = sock;
	op->complete = std::move(cb);

	// start write operation
	data.len = len;
	data.buf = buf;
	if(WSASend(sock->fd, &data, 1, &transfered, 0,
			(OVERLAPPED*)op, nullptr) == SOCKET_ERROR){
		error = GetLastError();
		if(error == WSA_IO_PENDING){
			op->opcode.store(OP_NONE, std::memory_order_relaxed);
			LOG_ERROR("socket_async_write: WSASend failed with error `%d`", error);
			return false;
		}
	}
	return true;
}

#endif //_WIN32
