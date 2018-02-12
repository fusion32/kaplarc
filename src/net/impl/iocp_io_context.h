#ifndef NET_IMPL_IOCP_IO_CONTEXT_H_
#define NET_IMPL_IOCP_IO_CONTEXT_H_

#include "../config.h"
#ifdef NET_PLATFORM_WINDOWS

#include <atomic>
#include <functional>
#include <system_error>
#include "winsock.h"

namespace net{
extern LPFN_ACCEPTEX			accept_ex;
extern LPFN_GETACCEPTEXSOCKADDRS	get_accept_ex_sockaddrs;

class iocp_io_context{
private:
	// internal context state
	HANDLE		iocp;
	bool		running;
	std::error_code	error;

	// delete copy and move operations
	iocp_io_context(const iocp_io_context&) = delete;
	iocp_io_context(iocp_io_context&&) = delete;
	iocp_io_context &operator=(const iocp_io_context&) = delete;
	iocp_io_context &operator=(iocp_io_context&&) = delete;

public:
	iocp_io_context(void);
	~iocp_io_context(void);

	bool is_running(void) const{
		return running;
	}
	const std::error_code &get_error(void) const{
		return error;
	}

	void work(void);
	void run(void);
	void stop(void);
};

}; //namespace net
#endif //NET_PLATFORM_WINDOWS
#endif //NET_IMPL_IOCP_IO_CONTEXT_H_
