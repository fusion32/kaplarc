#include "../../src/log.h"
#include "../../src/crypto/rsa.h"
const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
const char* e("65537");
int main(int argc, char **argv){
	struct rsa_ctx rsa;
	size_t len;
	char msg[255] = "MESSAGE";
	rsa_init(&rsa);
	rsa_setkey(&rsa, p, q, e);
	rsa_encode(&rsa, msg, strlen(msg), &len);
	LOG("encoded message: %llu -> %*s", len, len, msg);
	rsa_decode(&rsa, msg, len, &len);
	LOG("decoded message: %llu -> %*s", len, len, msg);
	rsa_clear(&rsa);
	return 0;
}
