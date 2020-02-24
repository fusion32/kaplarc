#ifndef DATABASE_DATABASE_H_
#define DATABASE_DATABASE_H_

#include "../def.h"

bool database_init(void);
void database_shutdown(void);

void database_load(void);
void database_store(void);

void database_serialize(void);
void database_stage(void);
void database_commit(void);

#endif //DATABASE_DATABASE_H_
