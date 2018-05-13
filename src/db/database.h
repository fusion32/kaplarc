#ifndef DB_DATABASE_H_
#define DB_DATABASE_H_

#include "../def.h"

bool database_init(void);
void database_shutdown(void);

void database_load(void);
void database_store(void);

#endif //DB_DATABASE_H_
