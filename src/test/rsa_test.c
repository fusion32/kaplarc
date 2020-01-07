// REMAKE
#if 0

#include "../def.h"
#include "../log.h"
#include "../crypto/rsa.h"

static const char p[] =
	"142996239624163995200701773828988955507954033454661532174705160829"
	"347375827760388829672133862046006741453928458538592179906264509724"
	"52084065728686565928113";
static const char q[] =
	"763097919597040472189120184779200212553540129277912393720744757459"
	"669278851364717923533552930725135057072840737370556470887176203301"
	"7096809910315212884101";
static const char e[] = "65537";

bool rsa_test(void){
	struct rsa_ctx rsa;
	size_t len;
	char msg[255] = "MESSAGE";

	rsa_init(&rsa);
	rsa_setkey(&rsa, p, q, e);
	rsa_encode(&rsa, msg, strlen(msg), &len); msg[len] = 0;
	LOG("encoded message: %llu -> %s", len, msg);
	rsa_decode(&rsa, msg, len, &len); msg[len] = 0;
	LOG("decoded message: %llu -> %s", len, msg);
	rsa_cleanup(&rsa);
	return false;
}

#endif
