#include "../def.h"
#ifdef BUILD_TEST

#include "../log.h"
#include <stdio.h>


#define EXTERN_TEST(name) extern bool name##_test(void);
#define RUN_TEST(name)						\
	do{	extern bool name##_test(void);			\
		if(name##_test()) LOG(#name "_test: passed");	\
		else LOG(#name "_test: failed"); }while(0)


int main(int argc, char **argv){
	//RUN_TEST(base64);
	//RUN_TEST(bcrypt);
	//RUN_TEST(blowfish);
	//RUN_TEST(rsa);
	//RUN_TEST(xtea);

	//RUN_TEST(rbtree);
	RUN_TEST(slab);
	RUN_TEST(slab_cache);
	LOG("all tests complete");
	getchar();
	return 0;
}

#endif //BUILD_TEST
