#include "iocp_io_context.h"

#ifdef NET_PLATFORM_WINDOWS

#include "../../throw.h"
#include "iocp_async_op.h"

namespace net{

/*************************************

	wsa_init

*************************************/
LPFN_ACCEPTEX			accept_ex = nullptr;
LPFN_GETACCEPTEXSOCKADDRS	get_accept_ex_sockaddrs = nullptr;
class wsa_init{
private:
	static WSAData data;
	static std::error_code load_extensions(void){
		SOCKET fd = INVALID_SOCKET;
		GUID guid0 = WSAID_ACCEPTEX;
		GUID guid1 = WSAID_GETACCEPTEXSOCKADDRS;
		DWORD dummy;

		// create dummy socket
		if((fd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr,
				0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
			goto error;

		// load AcceptEx
		if(WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid0, sizeof(GUID),
				&accept_ex, sizeof(LPFN_ACCEPTEX),
				&dummy, nullptr, nullptr) == SOCKET_ERROR)
			goto error;

		// load GetAcceptExSockaddrs
		if(WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid1, sizeof(GUID),
				&get_accept_ex_sockaddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
				&dummy, nullptr, nullptr) == SOCKET_ERROR)
			goto error;

		return std::error_code();

	error:
		// handle error
		auto ec = std::error_code(WSAGetLastError(),
				std::system_category());
		if(fd != INVALID_SOCKET)
			closesocket(fd);
		accept_ex = nullptr;
		get_accept_ex_sockaddrs = nullptr;
		return ec;
	}

	static void init(void){
		std::error_code err;
		if(WSAStartup(MAKEWORD(2, 2), &data) != 0){
			err = std::error_code(WSAGetLastError(),
					std::system_category());
			throw_error(err);
		}

		// load extensions
		err = load_extensions();
		if(err) throw_error(err);
	}

	static void cleanup(void){
		WSACleanup();
	}

public:
	wsa_init(void){ init(); }
	~wsa_init(void){ cleanup(); }
};

/*************************************

	iocp_io_context

*************************************/
iocp_io_context::iocp_io_context(void)
 : running(false), error() {
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if(iocp == nullptr){
		error = std::error_code(WSAGetLastError(),
			std::system_category());
	}
}

iocp_io_context::~iocp_io_context(void){
	if(iocp != nullptr){
		CloseHandle(iocp);
		iocp = nullptr;
	}
}

void iocp_io_context::work(void){
	async_op *op;
	DWORD transfered, error;
	ULONG_PTR completion_key;
	int dummy;
	BOOL ret;

	ret = GetQueuedCompletionStatus(iocp, &transfered,
		&completion_key, (OVERLAPPED**)&op, INFINITE);

	error = NO_ERROR;
	if(ret == FALSE)
		error = GetLastError();

	if(op == nullptr){
		if(error == WAIT_TIMEOUT) return 0;
		return posix_error(error);
	}

	if(op->opcode == OP_ACCEPT){
		pfnGetAcceptExSockaddrs(op->socket->addr_buffer, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&op->socket->local_addr, &dummy,
			(sockaddr**)&op->socket->remote_addr, &dummy);
	}

	op->complete(posix_error(error), transfered);
	// release std::function resources
	op->complete = nullptr;
	op->opcode.store(OP_NONE, std::memory_order_relaxed);
	return 0;
}

void iocp_io_context::run(void){
	while(running) work();
}

void iocp_io_context::stop(void){
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

}; //namespace net

#endif //NET_PLATFORM_WINDOWS
