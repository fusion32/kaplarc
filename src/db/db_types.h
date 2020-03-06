#ifndef DB_TYPES_H_
#define DB_TYPES_H_

#include "../def.h"

struct db_date{
	int year, month, day;
};

struct db_result_account{
	struct db_date premend;
	char password[64];
	char charlist[256];
};

/*
struct db_result_player{

};
*/

#endif // DB_TYPES_H_
