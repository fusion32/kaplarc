#ifndef DB_TYPES_H_
#define DB_TYPES_H_

#include "../def.h"

struct db_result_account_login{
	int64 premend;
	char password[64];
	char charlist[256];
};

/*
struct db_result_player{

};
*/

#endif // DB_TYPES_H_
