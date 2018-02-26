#include "../../src/log.h"
#include "../../src/crypto/xtea.h"
static char plain_msg[] = "MESSAGE";
static uint32 key[] = { 0x11111111, 0x22222222,
			0x33333333, 0x44444444 };
int main(int argc, char **argv){
	char msg[32]; strcpy(msg, plain_msg);
	uint32 len = array_size(plain_msg); // this includes the null terminator
	xtea_encode(key, (uint8*)msg, len);
	xtea_decode(key, (uint8*)msg, len);
	if(strcmp(msg, plain_msg) == 0)
		LOG("XTEA encryption test OK");
	else
		LOG("XTEA encryption test FAILED");
	getchar();
	return 0;
}