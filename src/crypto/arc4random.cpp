#include "arc4random.h"

//TODO: use the same algorithm as the original arc4random
// (see OpenBSD source tree for arc4random.c)

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")

static HCRYPTPROV handle = NULL;
void open_handle(void){
	if(handle != NULL)
		return;
	CryptAcquireContextA(&handle, NULL, NULL,
		PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
}
uint32 arc4random(void){
	uint32 x;
	open_handle();
	CryptGenRandom(handle, 4, (BYTE*)&x);
	return x;
}
void arc4random_buf(void *data, size_t len){
	open_handle();
	CryptGenRandom(handle, (DWORD)len, (BYTE*)data);
}

#elif PLATFORM_LINUX
#endif
