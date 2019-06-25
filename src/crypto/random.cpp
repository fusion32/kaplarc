#include "../def.h"
#include "random.h"

#if defined(PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wincrypt.h>
#include <mutex>
#pragma comment(lib, "advapi32.lib")

static std::mutex mtx;
static HCRYPTPROV prov = 0;
bool crypto_random(void *data, size_t len){
	if(len == 0) return true;
	std::unique_lock<std::mutex> lock(mtx);
	if(prov == 0){
		BOOL ok = CryptAcquireContextA(&prov, NULL, NULL,
			PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
		if(ok == FALSE)
			return false;
	}
	lock.unlock();
	return CryptGenRandom(prov, (DWORD)len, (BYTE*)data) != FALSE;
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
