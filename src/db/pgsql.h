#ifndef DB_PGSQL_H_
#define DB_PGSQL_H_

#include "db_types.h"

// init/shutdown
bool pgsql_init(void);
void pgsql_shutdown(void);

// loading functions
bool pgsql_load_account_login(const char *accname,
		struct db_result_account_login *acc);

// storing functions

#endif //DB_PGSQL_H_
