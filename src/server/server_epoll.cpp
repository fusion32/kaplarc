#include "server.h"
#ifdef PLATFORM_LINUX
#include "../log.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/* NOTE: on Linux, if the LINGER option is
 * enabled on a socket, a close system call
 * issued on that socket will block until
 * all pending data is drained REGARDLESS
 * if it's in a non blocking mode or not.
 */

struct Connection{
	int		fd;
	uint32		flags;
	uint32		rdwr_count;
	pthread_mutex_t	mtx;

	Message		*input;
	OutputMessage	*output;
};

#define SERVICE_MAX_PROTOCOLS 4
struct Service{
	int		fd;
	int		port;
	int		protocol_count;
	Protocol	protocols[SERVICE_MAX_PROTOCOLS];
};

/*
 * Socket Helpers
 */

static int fd_set_linger(int fd, int seconds){
	struct linger l;
	l.l_onoff = (seconds > 0);
	l.l_linger = seconds;
	if(setsockopt(fd, SOL_SOCKET, SO_LINGER,
			&l, sizeof(struct linger)) == -1){
		LOG_ERROR("fd_set_linger: failed to"
			" set linger option (errno = %d)", errno);
		return -1;
	}
	return 0;
}

static int fd_set_non_blocking(int fd){
	int flags = fcntl(fd, F_GETFL);
	if(flags == -1){
		LOG_ERROR("fd_set_non_blocking: failed to"
			" retrieve socket flags (errno = %d)", errno);
		return -1;
	}
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
		LOG_ERROR("fd_set_non_blocking: failed to"
			" set non blocking mode (errno = %d)", errno);
		return -1;
	}
	return 0;
}


/*
 * Connection Interface
 */

/*
 * Service Interface
 */



/*
 * Server Interface
 */

#define MAX_SERVICES 4
static bool	running = false;
static int	service_count = 0;
static Service	services[MAX_SERVICES];

bool server_add_protocol(Protocol *protocol, int port){
	int i;
	Service *s = nullptr;
	if(running || protocol == nullptr)
		return false;
	for(i = 0; i < service_count; i += 1){
		if(services[i].port == port){
			s = &services[i];
			break;
		}
	}

	if(s == nullptr){
		if(service_count >= MAX_SERVICES)
			return false;
		s = &services[service_count];
		service_count += 1;

		s->fd = -1;
		s->port = port;
		s->protocol_count = 1;
		memcpy(&s->protocols[0], protocol, sizeof(Protocol));
	}else{
		if(protocol->sends_first ||
		  s->protocol_count >= SERVICE_MAX_PROTOCOLS ||
		  s->protocols[0].sends_first)
			return false;
		i = s->protocol_count;
		s->protocol_count += 1;
		memcpy(&s->protocols[i], protocol, sizeof(Protocol));
	}
	return true;
}

void server_stop(void){
	running = false;
}

void server_run(void){
}

#endif
