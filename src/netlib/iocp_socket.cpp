#ifdef _WIN32
#include "iocp_private.h"
#include "../log.h"

static SocketOp *socket_op(Socket *sock, int opcode){
	int cmp;
	for(int i = 0; i < MAX_OPS; ++i){
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
	for(int i = 0; i < MAX_OPS; ++i){
		opcode = sock->ops[i].opcode.load(std::memory_order_acquire);
		if((how == SOCKET_SHUT_RDWR && opcode != OP_NONE)
			|| (how == SOCKET_SHUT_RD && opcode == OP_READ)
			|| (how == SOCKET_SHUT_RD && opcode == OP_ACCEPT)
			|| (how == SOCKET_SHUT_WR && opcode == OP_WRITE)){
			CancelIoEx((HANDLE)sock->fd, (OVERLAPPED*)&sock->ops[i]);
			sock->ops[i].complete = nullptr;
		}

	}
}

void socket_close(Socket *sock){
	CancelIoEx((HANDLE)sock->fd, nullptr);
	for(int i = 0; i < MAX_OPS; ++i)
		sock->ops[i].complete = nullptr;
	closesocket(sock->fd);
	delete sock;
}

bool socket_async_accept(Socket *sock, const SocketCallback &cb){
	return socket_async_accept(sock, SocketCallback(cb));
}

bool socket_async_read(Socket *sock, char *buf, int len, const SocketCallback &cb){
	return socket_async_read(sock, buf, len, SocketCallback(cb));
}

bool socket_async_write(Socket *sock, char *buf, int len, const SocketCallback &cb){
	return socket_async_write(sock, buf, len, SocketCallback(cb));
}

bool socket_async_accept(Socket *sock, SocketCallback &&cb){
	SocketOp *op;
	BOOL ret;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_ACCEPT);
	if(op == nullptr){
		LOG_ERROR("socket_async_accept: maximum simultaneous operations reached (%d)", MAX_OPS);
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
	ret = pfnAcceptEx(sock->fd, op->socket->fd, op->socket->addr_buffer, 0,
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

bool socket_async_read(Socket *sock, char *buf, int len, SocketCallback &&cb){
	SocketOp *op;
	WSABUF data;
	DWORD flags;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_READ);
	if(op == nullptr){
		LOG_ERROR("socket_async_read: maximum simultaneous operations reached (%d)", MAX_OPS);
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

bool socket_async_write(Socket *sock, char *buf, int len, SocketCallback &&cb){
	SocketOp *op;
	WSABUF data;
	DWORD transfered;
	DWORD error;

	// retrieve op handle
	op = socket_op(sock, OP_WRITE);
	if(op == nullptr){
		LOG_ERROR("socket_async_write: maximum simultaneous operations reached (%d)", MAX_OPS);
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
