#include "server.h"
#ifdef PLATFORM_LINUX
#include "../log.h"

#include <errno.h>
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

class Connection{
	int		fd;
	uint32		flags;
	uint32		rdwr_count;
	pthread_mutex_t	mtx;

	Message		*input;
	OutputMessage	*output;
};

static int fd_set_linger(int fd, int seconds){
	struct linger l;
	l.l_onoff = (seconds > 0);
	l.l_linger = seconds;
	if(setsockopt(fd, SOL_SOCKET, SO_LINGER,
			&linger, sizeof(struct linger)) == -1){
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

#endif
