#include "bcrypt.h"
#include "blowfish.h"
#include "base64.h"
#include "arc4random.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// x -> log_rounds		(2 bytes)
// y -> salt encoded in base 64	(22 bytes)
// z -> hash encoded in base 64	(32 bytes)
// hash total length = 62 bytes (including null terminator)
// salt total length = 30 bytes (including null terminator)
// "$2b$x$y[z]"

#define BCRYPT_SALT_LEN		16
#define BCRYPT_HASH_LEN		24
#define BCRYPT_SALT_STRLEN	30
#define BCRYPT_HASH_STRLEN	62

static bool generate_salt(int logr, char *salt, size_t saltlen){
	uint8 csalt[BCRYPT_SALT_LEN];
	if(saltlen < BCRYPT_SALT_STRLEN)
		return false;
	if(logr < 4)
		logr = 4;
	else if(logr > 31)
		logr = 31;
	arc4random_buf(csalt, BCRYPT_SALT_LEN);
	snprintf(salt, saltlen, "$2b$%2.2u$", logr);
	base64_encode(salt + 7, csalt, BCRYPT_SALT_LEN); 
	return true;
}

static bool generate_hash(const char *key, const char *salt,
			char *hash, size_t hashlen){
	static const char ptext[] =
		"OrpheanBeholderScryDoubt";
	struct blowfish_ctx b;
	uint8 csalt[BCRYPT_SALT_LEN];
	uint8 ctext[BCRYPT_HASH_LEN];
	uint8 minor, logr;
	uint32 rounds, i;
	size_t keylen;

	if(strlen(salt)+1 < BCRYPT_SALT_STRLEN)
		return false;

	if(salt[0] != '$' || salt[1] != '2')
		return false;

	// include null terminator on key length
	keylen = strlen(key) + 1;
	switch((minor = salt[2])){
	case 'b':	if(keylen > 72)
				keylen = 72;
	case 'a':	break;
	default:	return false;
	}

	// check rounds
	if(salt[3] != '$' || salt[6] != '$' ||
	   isdigit(salt[4]) == 0 || isdigit(salt[5]) == 0)
		return false;
	logr = (salt[4] - '0') * 10 + (salt[5] - '0');
	if(logr < 4 || logr > 31)
		return false;
	rounds = 1UL << logr;

	// decode salt from salt string
	if(!base64_decode(csalt, BCRYPT_SALT_LEN, salt + 7))
		return false;

	// setup key
	blowfish_init(&b);
	blowfish_expandkey1(&b, (uint8*)key, keylen,
		csalt, BCRYPT_SALT_LEN);
	for(i = 0; i < rounds; i++){
		blowfish_expandkey(&b, (uint8*)key, keylen);
		blowfish_expandkey(&b, csalt, BCRYPT_SALT_LEN);
	}

	// encrypt
	memcpy(ctext, ptext, BCRYPT_HASH_LEN);
	blowfish_ecb_encode_n(&b, 64, ctext, BCRYPT_HASH_LEN);

	// assemble the hash string
	snprintf(hash, hashlen, "$2%c$%2.2u$", minor, logr);
	base64_encode(hash + 7, csalt, BCRYPT_SALT_LEN);
	base64_encode(hash + 7 + 22, ctext, BCRYPT_HASH_LEN);
	// base64_encode already adds the null terminator
	return true;
}

static bool hash_cmp(const char *h1, const char *h2, size_t len){
	int res, i;
	for(res = 0, i = 0; i < len; i++)
		res += h1[i] ^ h2[i];
	return res == 0;
}

bool bcrypt_newhash(const char *pass, int logr, char *hash, size_t hashlen){
	char salt[BCRYPT_SALT_STRLEN];
	if(!generate_salt(logr, salt, BCRYPT_SALT_STRLEN))
		return false;
	if(!generate_hash(pass, salt, hash, hashlen))
		return false;
	return true;
}

bool bcrypt_checkpass(const char *pass, const char *hash){
	char test_hash[BCRYPT_HASH_STRLEN];
	size_t hashlen;

	if(!generate_hash(pass, hash, test_hash, BCRYPT_HASH_STRLEN))
		return false;
	hashlen = strlen(hash);
	if(hashlen != strlen(test_hash) ||
	   !hash_cmp(hash, test_hash, hashlen))
		return false;
	return true;
}
