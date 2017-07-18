#ifndef NETWORK_H_
#define NETWORK_H_

#include <functional>
#include "../def.h"

#define NET_WORK_TIMEOUT	1000

#define SOCKET_SHUT_RD		0x00
#define SOCKET_SHUT_WR		0x01
#define SOCKET_SHUT_RDWR	0x02

struct Socket;
using SocketCallback = std::function<void(Socket*, int, int)>;

// network service
bool	net_init(void);
void	net_shutdown(void);
Socket	*net_socket(void);
Socket	*net_server_socket(int port);
int	net_work(void);

// socket control
uint32	socket_remote_address(void);
void	socket_shutdown(Socket *sock, int how);
void	socket_close(Socket *sock);
bool	socket_async_accept(Socket *sock, SocketCallback cb);
bool	socket_async_read(Socket *sock, char *buf, int len, SocketCallback cb);
bool	socket_async_write(Socket *sock, char *buf, int len, SocketCallback cb);

#endif //NETWORK_H_
