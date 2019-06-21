#include "../crypto/rsa.h"
#include "../def.h"
#include "../log.h"
#include "server_rsa.h"
#include <mutex>

static const char p[] =
	"142996239624163995200701773828988955507954033454661532174705160829"
	"347375827760388829672133862046006741453928458538592179906264509724"
	"52084065728686565928113";
static const char q[] =
	"763097919597040472189120184779200212553540129277912393720744757459"
	"669278851364717923533552930725135057072840737370556470887176203301"
	"7096809910315212884101";
static const char e[] = "65537";

static std::mutex mtx;
static struct rsa_ctx ctx;
static bool initialized = false;

bool server_rsa_init(void){
	std::lock_guard<std::mutex> guard(mtx);
	if(initialized)
		return true;
	rsa_init(&ctx);
	if(!rsa_setkey(&ctx, p, q, e)){
		LOG_ERROR("grsa_init: failed to set key");
		rsa_cleanup(&ctx);
		return false;
	}
	initialized = true;
	return true;
}

void server_rsa_shutdown(void){
	std::lock_guard<std::mutex> guard(mtx);
	if(initialized){
		rsa_cleanup(&ctx);
		initialized = false;
	}
}

bool server_rsa_encode(uint8 *data, size_t len, size_t *plen){
	std::lock_guard<std::mutex> guard(mtx);
	if(ctx.encoding_limit < len)
		return false;
	rsa_encode(&ctx, data, len, plen);
	return true;
}

bool server_rsa_decode(uint8 *data, size_t len, size_t *plen){
	std::lock_guard<std::mutex> guard(mtx);
	if(ctx.encoding_limit < len)
		return false;
	rsa_decode(&ctx, data, len, plen);
	return true;
}