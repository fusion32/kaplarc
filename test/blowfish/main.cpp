#include "../../src/log.h"
#include "../../src/crypto/blowfish.h"
#define round_to_2(x) (((x) + 1) & ~1)
int main(int argc, char **argv){
	blowfish_ctx b;
	const char key[] = "KEYTEST";
	char msg[255] = "MESSAGE";
	uint32 len = round_to_2(strlen(msg) / 4);
	blowfish_setkey(&b, key, strlen(key));
	LOG("plain text: %s", msg);
	blowfish_encode_le(&b, (uint32*)msg, len);
	LOG("encoded: %s", msg);
	blowfish_decode_le(&b, (uint32*)msg, len);
	LOG("decoded: %s", msg);
	getchar();
	return 0;
}