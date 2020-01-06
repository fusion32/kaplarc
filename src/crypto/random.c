#include "../def.h"
#include "random.h"

#if defined(PLATFORM_WINDOWS)
// TODO
bool crypto_random(void *data, size_t len){
	ASSERT(0 && "crypto_random: not implemented");
	return len == 0;
}

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_FREEBSD)

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>

static bool getrandom_wrapper(void *data, size_t len){
	ssize_t ret;
	while(len > 0){
		ret = getrandom(data, len, 0);
		if(ret == -1)
			return false;
		len -= ret;
		data = advance_pointer(data, ret);
	}
	return true;
}

static bool urandom_wrapper(void *data, size_t len){
	static int fd = -1;
	static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

	ssize_t ret;
	pthread_mutex_lock(&mtx);
	if(fd == -1){
		fd = open("/dev/urandom", O_RDONLY);
		if(fd == -1){
			pthread_mutex_unlock(&mtx);
			return false;
		}
	}
	while(len > 0){
		ret = read(fd, data, len);
		if(ret == -1){
			pthread_mutex_unlock(&mtx);
			return false;
		}
		len -= ret;
		data = advance_pointer(data, ret);
	}
	pthread_mutex_unlock(&mtx);
	return true;
}

bool crypto_random(void *data, size_t len){
	if(getrandom_wrapper(data, len))
		return true;
	if(errno != ENOSYS)
		return false;
	return urandom_wrapper(data, len);
}

#endif
