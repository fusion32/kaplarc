// REMAKE
#if 0

#include "../log.h"
#include "../crypto/bcrypt.h"

static const char pw[] = "h4rdp455w0rd";
bool bcrypt_test(void){
	char hash[255];
	LOG("password: %s", pw);
	if(!bcrypt_newhash(pw, 10, hash, 255)){
		LOG_ERROR("failed to create hash");
		return false;
	}
	LOG("hash: %s", hash);
	if(bcrypt_checkpass(pw, hash))
		LOG("password hash check passed");
	else
		LOG("password hash check failed");
	return false;
}

#endif
