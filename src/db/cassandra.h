#ifndef DB_CASSANDRA_H_
#define DB_CASSANDRA_H_

#include <cassandra.h>

bool cass_init(void);
void cass_shutdown(void);

void cass_test(void);

#endif //DB_CASSANDRA_H_
