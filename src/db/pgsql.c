#include "pgsql.h"
#include "../buffer_util.h"
#include "../config.h"
#include "../log.h"

#include <libpq-fe.h>

static struct{
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
	// @TODO enable SSL
	{"pgsql_sslmode",		"sslmode"},
	{"pgsql_sslcert",		"sslcert"},
	{"pgsql_sslkey",		"sslkey"},
	{"pgsql_sslrootcert",		"sslrootcert"},
	{"pgsql_sslcrl",		"sslcrl"},
*/
};
#define NUM_PARAMS ARRAY_SIZE(pgsql_params)

static PGconn *conn = NULL;
bool pgsql_init(void){
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
		LOG_ERROR("pgsql_init: failed to create connection handle");
		return false;
	}
	if(PQstatus(conn) != CONNECTION_OK){
		LOG_ERROR("pgsql_init: connection failed: %s",
			PQerrorMessage(conn));
		conn = NULL;
		return false;
	}
	return true;
}

void pgsql_shutdown(void){
	DEBUG_ASSERT(conn != NULL);
	PQfinish(conn);
	conn = NULL;
}

// binary int/float is sent in network byte order
static int32 pq_getint32(const char *data){
	return decode_u32_be((uint8*)data);
}
static int64 pq_getint64(const char *data){
	return decode_u64_be((uint8*)data);
}
static float pq_getfloat(const char *data){
	return decode_f32_be((uint8*)data);
}
static double pq_getdouble(const char *data){
	return decode_f64_be((uint8*)data);
}

bool pgsql_load_account(const char *accname, struct db_result_account *acc){
	char account_id[4];
	PGresult *res;
	const char *param_value;
	int param_length;
	int param_format;
	int ntuples;

	// load account info
	param_value = accname;
	res = PQexecParams(conn,
		"SELECT"
			" account_id,"
			" date_part('epoch', premend)::bigint,"
			" password"
		" FROM accounts WHERE lower(name) = $1",
		1, NULL, &param_value, NULL, NULL, 1);
	if(res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK){
		LOG_ERROR(PQerrorMessage(conn));
		if(res != NULL)
			PQclear(res);
		return false;
	}
	memcpy(account_id, PQgetvalue(res, 0, 0), 4);
	acc->premend = pq_getint64(PQgetvalue(res, 0, 1));
	kpl_strncpy(acc->password, sizeof(acc->password),
		PQgetvalue(res, 0, 2));
	PQclear(res);

	// load charlist
	param_value = account_id;
	param_length = 4; // sizeof(int32)
	param_format = 1; // binary
	res = PQexecParams(conn,
		"SELECT name FROM players"
		" WHERE account_id = $1"
		" ORDER BY name ASC",
		1, NULL, &param_value, &param_length, &param_format, 1);
	if(res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK){
		LOG_ERROR(PQerrorMessage(conn));
		if(res != NULL)
			PQclear(res);
		return false;
	}
	ntuples = PQntuples(res);
	if(ntuples <= 0){
		acc->charlist[0] = 0;
	}else{
		kpl_strncpy(acc->charlist,
			sizeof(acc->charlist),
			PQgetvalue(res, 0, 0));
		for(int i = 1; i < ntuples; i += 1){
			kpl_strncat_n(acc->charlist,
				sizeof(acc->charlist),
				";", PQgetvalue(res, i, 0), NULL);
		}
	}
	PQclear(res);
	return true;
}

/*
void pgsql_set_account_premend(int32 account_id, int32 premend){
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

