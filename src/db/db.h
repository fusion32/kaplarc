#ifndef DB_DB_H_
#define DB_DB_H_

#include "../def.h"

bool db_init(void);
void db_shutdown(void);

void db_load(void);
void db_store(void);

void db_serialize(void);
void db_stage(void);
void db_commit(void);

#endif //DB_DATABASE_H_
