
// NOTE1: any db_load_* functions should be run in a task
// running on the database thread

// NOTE2: DB_RESULT_* values are intimately connected to
// the query functions associated and should only be changed
// accordingly.

#ifndef KAPLAR_DB_DATABASE_H_
#define KAPLAR_DB_DATABASE_H_ 1

#include "../common.h"

typedef void db_result_t;

// internal database routines
#ifdef DB_INTERNAL
bool db_internal_connect(void);
bool db_internal_connection_reset(void);
void db_internal_connection_close(void);
#endif

// init/shutdown
bool db_init(void);
void db_shutdown(void);
bool db_add_task(void (*fp)(void*), void *udata);

// db result interface
int db_result_nrows(db_result_t *res);
int32 db_result_get_int32(db_result_t *res, int row, int field);
int64 db_result_get_int64(db_result_t *res, int row, int field);
float db_result_get_float(db_result_t *res, int row, int field);
double db_result_get_double(db_result_t *res, int row, int field);
const char *db_result_get_value(db_result_t *res, int row, int field);
void db_result_clear(db_result_t *res);

// account loading functions
#define DB_RESULT_ACCOUNT_INFO_ID	0
#define DB_RESULT_ACCOUNT_INFO_PREMEND	1
#define DB_RESULT_ACCOUNT_INFO_PASSWORD	2
db_result_t *db_load_account_info(const char *name);
db_result_t *db_load_account_charlist(int32 account_id);

void db_print_account(const char *name);

// player loading functions


// storing functions

#endif //KAPLAR_DB_DATABASE_H_
