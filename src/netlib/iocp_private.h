#ifdef _WIN32
#ifndef IOCP_PRIVATE_H_
#define IOCP_PRIVATE_H_

#define WIN32_LEAN_AND_MEAN 1
#include <atomic>
#include <errno.h>
#include <winsock2.h>
#include <mswsock.h>

#include "network.h"

#define MAX_OPS		8
#define OP_NONE		0x00
#define OP_ACCEPT	0x01
#define OP_READ		0x02
#define OP_WRITE	0x03

struct SocketOp{
	OVERLAPPED		overlapped;
	std::atomic<int>	opcode;
	Socket			*socket;
	SocketCallback		complete;
};

struct Socket{
	SOCKET		fd;
	SocketOp	ops[MAX_OPS];
	char		addr_buffer[sizeof(sockaddr_in) * 2 + 16];
	sockaddr_in	*local_addr;
	sockaddr_in	*remote_addr;
};


extern	LPFN_ACCEPTEX			pfnAcceptEx;
extern	LPFN_GETACCEPTEXSOCKADDRS	pfnGetAcceptExSockaddrs;

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

#endif // IOCP_PRIVATE_H_
#endif //_WIN32
