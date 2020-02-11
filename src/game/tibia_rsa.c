#include "../crypto/rsa.h"
#include "../log.h"
#include "tibia_rsa.h"

static const char p[] =
	"142996239624163995200701773828988955507954033454661532174705160829"
	"347375827760388829672133862046006741453928458538592179906264509724"
	"52084065728686565928113";
static const char q[] =
	"763097919597040472189120184779200212553540129277912393720744757459"
	"669278851364717923533552930725135057072840737370556470887176203301"
	"7096809910315212884101";
static const char e[] = "65537";

// We will use only RSA decoding and it'll be done only
// on the server thread when receiving the first message
// of the login or game protocols. This implies that
// locks are not required.
static struct rsa_ctx ctx;

bool tibia_rsa_init(void){
	rsa_init(&ctx);
	if(!rsa_setkey(&ctx, p, q, e)){
		LOG_ERROR("tibia_rsa_init: failed to set key");
		rsa_cleanup(&ctx);
		return false;
	}
	return true;
}

void tibia_rsa_shutdown(void){
	rsa_cleanup(&ctx);
}

bool tibia_rsa_encode(uint8 *data, size_t len, size_t *outlen){
	if(ctx.encoding_limit < len)
		return false;
	rsa_encode(&ctx, data, len, outlen);
	return true;
}

bool tibia_rsa_decode(uint8 *data, size_t len, size_t *outlen){
	if(ctx.encoding_limit < len)
		return false;
	rsa_decode(&ctx, data, len, outlen);
	return true;
}
