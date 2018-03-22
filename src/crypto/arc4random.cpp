#include "arc4random.h"

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <mutex>
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")

static std::mutex	mtx;
static HCRYPTPROV	prov = 0;
void arc4random_buf(void *data, size_t len){
	// initialize if needed
	{	std::lock_guard<std::mutex> g(mtx);
		if(prov == 0){
			BOOL ok = CryptAcquireContextA(&prov, NULL, NULL,
				PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
			if(ok == FALSE)
				return;
		}
	}

	CryptGenRandom(prov, (DWORD)len, (BYTE*)data);
}

uint32 arc4random(void){
	uint32 val;
	arc4random_buf(&val, 4);
	return val;
}

#elif PLATFORM_LINUX
#include <mutex>
#include <unistd.h>

static std::mutex	mtx;
static int		fd = 0;
void arc4random_buf(void *data, size_t len, int){
	std::lock_guard<std::mutex> g(mtx);
	if(fd == 0){
		fd = open("/dev/urandom", O_RDONLY);
		if(fd == 0)
			return;
	}
	read(fd, data, len);
}

uint32 arc4random(void){
	uint32 val;
	arc4random_buf(&val, 4);
	return val;
}

#endif
