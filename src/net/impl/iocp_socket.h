#ifdef _WIN32
#ifndef IOCP_PRIVATE_H_
#define IOCP_PRIVATE_H_

#define WIN32_LEAN_AND_MEAN 1
#include <atomic>
#include <errno.h>
#include <winsock2.h>
#include <mswsock.h>

#include "iocp_operation.h"


namespace net{

#define MAX_OPS		8
struct Socket{
	SOCKET		fd;
	op_queue	*rd_queue;
	op_queue	*wr_queue;
	op_queue	*usr_queue;

	char		addr_buffer[(sizeof(sockaddr_in) + 16) * 2];
	sockaddr_in	*local_addr;
	sockaddr_in	*remote_addr;
};

}; //namespace net

#endif // IOCP_PRIVATE_H_
#endif //_WIN32
