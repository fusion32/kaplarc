
// NOTE1: any db_load_*_async function runs the callback
// inside the database thread so inside the callback you
// can and should use the non-async versions of the load
// functions to get any additional data.

// NOTE2: DB_RESULT_* values are intimately connected to
// the query functions associated and should only be changed
// accordingly.

#ifndef KAPLAR_DB_DATABASE_H_
#define KAPLAR_DB_DATABASE_H_ 1

#include "../common.h"

// load operations
#define DB_LOAD_OP_ACCOUNT_INFO		1
#define DB_LOAD_OP_ACCOUNT_CHARLIST	2
#define DB_LOAD_OP_PLAYER		3

// load types
typedef union{
	int32		id;
	const char	*name;
} db_load_key_t;
typedef void db_result_t;
typedef void(*db_load_callback_t)(void *udata, uint32 load_op,
	db_load_key_t *key, db_result_t *res);

#if 0
//@TODO
// store operations
// store types
//typedef void(*db_store_callback_t)(void *udata, uint32 op_id, db_store_data_t *data);
#endif

// internal database routines
#ifdef DB_INTERNAL
bool db_internal_connect(void);
bool db_internal_connection_reset(void);
void db_internal_connection_close(void);
#endif

// init/shutdown
bool db_init(void);
void db_shutdown(void);

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
void db_load_account_info_async(const char *name,
	db_load_callback_t callback, void *udata);
void db_load_account_charlist_async(int32 account_id,
	db_load_callback_t callback, void *udata);

void db_print_account(const char *name);

// player loading functions


// storing functions

#endif //KAPLAR_DB_DATABASE_H_
