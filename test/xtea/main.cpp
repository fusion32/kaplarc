#include "../../src/log.h"
#include "../../src/crypto/xtea.h"
#define round_to_2(x) (((x) + 1) & ~1)
int main(int argc, char **argv){
	char msg[255] = "MESSAGE";
	uint32 len = round_to_2(strlen(msg) / 4);
	uint32 key[4] = {
		0x11111111,
		0x22222222,
		0x33333333,
		0x44444444,
	};
	LOG("plain: %s", msg);
	xtea_encode_le(key, (uint32*)msg, len);
	LOG("encoded: %s", msg);
	xtea_decode_le(key, (uint32*)msg, len);
	LOG("decoded: %s", msg);
	getchar();
	return 0;
}