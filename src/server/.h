#ifndef SERVER_SOCKET_H_
#define SERVER_SOCKET_H_
#include "../def.h"

#ifdef PLATFORM_WINDOWS
typedef uintptr socket_t;
#else
typedef int socket_t;
#endif

#endif //SERVER_SOCKET_H_
