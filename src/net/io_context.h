#ifndef NET_IO_CONTEXT_H_
#define NET_IO_CONTEXT_H_

#include "../config.h"
//#include "impl/epoll_io_context.h"
#include "impl/iocp_io_context.h"
//#include "impl/kqueue_io_context.h"

namespace net{

#if defined(NET_PLATFORM_WINDOWS)
using io_context = iocp_io_context;
#elif defined(NET_PLATFORM_LINUX)
using io_context = epoll_io_context;
#elif defined(NET_PLATFORM_BSD)
using io_context = kqueue_io_context;
#endif

}; // namespace net

#endif //NET_IO_CONTEXT_H_
