// REMAKE
#if 0

#include "../log.h"
#include "../crypto/blowfish.h"

static const char key[] = "KEYTEST";
static const char iv[] = "12345678"; // this must be at least 8 bytes long
static const char plain_msg[] = "MESSAGE THAT IS MORE THAN 8 BYTES LONG";
static const char salt[] = "abcdefghijklmno";

static bool test_ecb_blowfish(struct blowfish_ctx *b){
	char msg[64]; strcpy(msg, plain_msg);
	uint32 len = array_size(plain_msg); // this includes the null terminator
	blowfish_ecb_encode(b, (uint8*)msg, len);
	blowfish_ecb_decode(b, (uint8*)msg, len);
	return strcmp(msg, plain_msg) == 0;
}

static bool test_cbc_blowfish(struct blowfish_ctx *b){
	char msg[64]; strcpy(msg, plain_msg);
	uint32 len = array_size(plain_msg); // this includes the null terminator
	blowfish_cbc_encode(b, (uint8*)iv, (uint8*)msg, len);
	blowfish_cbc_decode(b, (uint8*)iv, (uint8*)msg, len);
	return strcmp(msg, plain_msg) == 0;
}

bool blowfish_test(void){
	struct blowfish_ctx b;

	blowfish_init(&b);
	for(int i = 0; i < 256; ++i)
		blowfish_expandkey(&b, (uint8*)key, array_size(key));
	for(int i = 0; i < 1024; ++i)
		blowfish_expandkey1(&b, (uint8*)key, array_size(key),
			(uint8*)salt, array_size(salt));
	if(test_ecb_blowfish(&b))
		LOG("blowfish ecb encryption test OK");
	else
		LOG("blowfish ecb encryption test FAILED");
	if(test_cbc_blowfish(&b))
		LOG("blowfish cbc encryption test OK");
	else
		LOG("blowfish cbc encryption test FAILED");
	getchar();
	return false;
}

#endif
