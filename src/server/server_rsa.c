#include "../crypto/rsa.h"
#include "../def.h"
#include "../log.h"
#include "../thread.h"
#include "server_rsa.h"

static const char p[] =
	"142996239624163995200701773828988955507954033454661532174705160829"
	"347375827760388829672133862046006741453928458538592179906264509724"
	"52084065728686565928113";
static const char q[] =
	"763097919597040472189120184779200212553540129277912393720744757459"
	"669278851364717923533552930725135057072840737370556470887176203301"
	"7096809910315212884101";
static const char e[] = "65537";

static struct rsa_ctx ctx;
static mutex_t mtx;

bool server_rsa_init(void){
	rsa_init(&ctx);
	if(!rsa_setkey(&ctx, p, q, e)){
		LOG_ERROR("grsa_init: failed to set key");
		rsa_cleanup(&ctx);
		return false;
	}
	if(mutex_init(&mtx) != 0){
		LOG_ERROR("server_rsa_init: failed to init mutex");
		rsa_cleanup(&ctx);
		return false;
	}
	return true;
}

void server_rsa_shutdown(void){
	mutex_destroy(&mtx);
	rsa_cleanup(&ctx);
}

bool server_rsa_encode(uint8 *data, size_t len, size_t *plen){
	mutex_lock(&mtx);
	if(ctx.encoding_limit < len){
		mutex_unlock(&mtx);
		return false;
	}
	rsa_encode(&ctx, data, len, plen);
	mutex_unlock(&mtx);
	return true;
}

bool server_rsa_decode(uint8 *data, size_t len, size_t *plen){
	mutex_lock(&mtx);
	if(ctx.encoding_limit < len){
		mutex_unlock(&mtx);
		return false;
	}
	rsa_decode(&ctx, data, len, plen);
	mutex_unlock(&mtx);
	return true;
}
