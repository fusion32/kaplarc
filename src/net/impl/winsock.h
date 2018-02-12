#ifndef NET_IMPL_WINSOCK_H_
#define NET_IMPL_WINSOCK_H_
#include "../config.h"
#ifdef NET_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN 1
#include <WinSock2.h>
#include <MSWSock.h>

#endif //NET_PLATFORM_WINDOWS
#endif //NET_IMPL_WINSOCK_H_
