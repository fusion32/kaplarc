
// NOTE1: any "format" param from the query functions can be
//	either `0` for string format or `1` for binary format
// NOTE2: binary int/float is sent in big endian

#define DB_INTERNAL 1

#include "database.h"
#include "../buffer_util.h"
#include "../config.h"
#include "../log.h"
#include "../thread.h"

#include <libpq-fe.h>

static const struct{
	const char *config_key;
	const char *param_key;
} pgsql_params[] = {
	{"pgsql_host",			"host"},
	{"pgsql_port",			"port"},
	{"pgsql_dbname",		"dbname"},
	{"pgsql_user",			"user"},
	{"pgsql_password",		"password"},
	{"pgsql_connect_timeout",	"connect_timeout"},
	{"pgsql_client_encoding",	"client_encoding"},
	{"pgsql_application_name",	"application_name"},

/*
	// @TODO enable SSL (IMPORTANT)
	{"pgsql_sslmode",		"sslmode"},
	{"pgsql_sslcert",		"sslcert"},
	{"pgsql_sslkey",		"sslkey"},
	{"pgsql_sslrootcert",		"sslrootcert"},
	{"pgsql_sslcrl",		"sslcrl"},
*/
};
#define NUM_PARAMS ARRAY_SIZE(pgsql_params)

// DB INTERNAL ROUTINES
static PGconn *conn = NULL;
bool db_internal_connect(void){
	DEBUG_ASSERT(conn == NULL);

	// k & v are null terminated arrays
	const char *k[NUM_PARAMS + 1];
	const char *v[NUM_PARAMS + 1];
	const char *config_key;
	for(int i = 0; i < NUM_PARAMS; i += 1){
		// assert pgsql_params array is not corrupted
		DEBUG_ASSERT(pgsql_params[i].config_key != NULL);
		DEBUG_ASSERT(pgsql_params[i].param_key != NULL);
		config_key = pgsql_params[i].config_key;
		k[i] = pgsql_params[i].param_key;
		v[i] = config_get(config_key);
		// assert the value string is not empty
		DEBUG_ASSERT(v[i][0] != 0);
	}
	k[NUM_PARAMS] = NULL;
	v[NUM_PARAMS] = NULL;

	conn = PQconnectdbParams(k, v, 0);
	if(conn == NULL){
		LOG_ERROR("db_connect: failed to create connection handle");
		return false;
	}
	if(PQstatus(conn) != CONNECTION_OK){
		LOG_ERROR("db_connect: %s", PQerrorMessage(conn));
		conn = NULL;
		return false;
	}
	return true;
}

bool db_internal_connection_reset(void){
	DEBUG_ASSERT(conn != NULL);
	PQreset(conn);
	if(PQstatus(conn) != CONNECTION_OK){
		LOG_ERROR("db_connection_reset: %s", PQerrorMessage(conn));
		return false;
	}
	return true;
}

void db_internal_connection_close(void){
	DEBUG_ASSERT(conn != NULL);
	PQfinish(conn);
	conn = NULL;
}

// db result interface
int db_result_nrows(db_result_t *res){
	return PQntuples(res);
}
int32 db_result_get_int32(db_result_t *res, int row, int field){
	return decode_u32_be(PQgetvalue(res, row, field));
}
int64 db_result_get_int64(db_result_t *res, int row, int field){
	return decode_u64_be(PQgetvalue(res, row, field));
}
float db_result_get_float(db_result_t *res, int row, int field){
	return decode_f32_be(PQgetvalue(res, row, field));
}
double db_result_get_double(db_result_t *res, int row, int field){
	return decode_f64_be(PQgetvalue(res, row, field));
}
const char *db_result_get_value(db_result_t *res, int row, int field){
	return PQgetvalue(res, row, field);
}
void db_result_clear(db_result_t *res){
	PQclear(res);
}

db_result_t *db_load_account_info(const char *accname){
	PGresult *res;
	res = PQexecParams(conn,
		"SELECT account_id, date_part('epoch', premend)::bigint, password"
		" FROM accounts"
		" WHERE lower(name) = $1",
		/*nParams = */		1,
		/*paramTypes = */	NULL,
		/*paramValues = */	&accname,
		/*paramLengths = */	NULL,
		/*paramFormats = */	NULL,
		/*resultFormat = */	1);
	if(res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK){
		LOG_ERROR("db_load_account_info: %s", PQerrorMessage(conn));
		if(res != NULL)
			PQclear(res);
		return NULL;
	}
	return res;
}

db_result_t *db_load_account_charlist(int32 account_id){
	PGresult *res;
	char param_buf[sizeof(int32)];
	char *param_value = param_buf;
	int param_length = sizeof(int32);
	int param_format = 1;
	encode_u32_be(param_buf, account_id);
	res = PQexecParams(conn,
		"SELECT name"
		" FROM players"
		" WHERE account_id = $1"
		" ORDER BY name ASC",
		/*nParams = */		1,
		/*paramTypes = */	NULL,
		/*paramValues = */	&param_value,
		/*paramLengths = */	&param_length,
		/*paramFormats = */	&param_format,
		/*resultFormat = */	1);
	if(res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK){
		LOG_ERROR("db_load_account_charlist: %s", PQerrorMessage(conn));
		if(res != NULL)
			PQclear(res);
		return NULL;
	}
	return res;
}

void db_print_account(const char *accname){
	int32 id;
	int64 premend;
	const char *pwd;
	int nrows;
	db_result_t *res = db_load_account_info(accname);
	if(res == NULL){
		LOG_ERROR("failed to load account");
		return;
	}
	id = db_result_get_int32(res, 0, 0);
	premend = db_result_get_int64(res, 0, 1);
	pwd = db_result_get_value(res, 0, 2);
	LOG("account: id = %d, name = `%s`", id, accname);
	LOG("> premend = %lld", premend);
	LOG("> password = %s", pwd);
	db_result_clear(res);

	res = db_load_account_charlist(id);
	if(res == NULL){
		LOG_ERROR("failed to load charlist");
		return;
	}
	nrows = db_result_nrows(res);
	if(nrows > 0){
		for(int i = 0; i < nrows; i += 1)
			LOG("> character #%d = `%s`", i, db_result_get_value(res, i, 0));
	}else{
		LOG("> character list empty");
	}
	db_result_clear(res);
}

/*
void pgsql_store_account_premend(int32 account_id, int32 premend){
	// $1 = premend;
	// $2 = account_id;
	UPDATE accounts
	SET premend = to_timestamp($1)::date + INTERVAL '1 day'
	WHERE account_id = $2;
}
*/

/*
bool pgsql_load_player(const char *name, struct db_result_player *player){
	// SELECT id, ... FROM players WHERE name = $1;
	// SELECT ... FROM player_vip WHERE id = $1;
	// SELECT ... FROM player_items WHERE id = $1;
	// SELECT ... FROM player_storage WHERE id = $1;
}
*/

