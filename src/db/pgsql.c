#include "pgsql.h"
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

static PGconn *pgsql_conn = NULL;
bool pgsql_init(void){
	DEBUG_ASSERT(pgsql_conn == NULL);

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

	pgsql_conn = PQconnectdbParams(k, v, 0);
	if(pgsql_conn == NULL){
		LOG_ERROR("pgsql_init: failed to create connection handle");
		return false;
	}
	if(PQstatus(pgsql_conn) != CONNECTION_OK){
		LOG_ERROR("pgsql_init: connection failed: %s",
			PQerrorMessage(pgsql_conn));
		pgsql_conn = NULL;
		return false;
	}
	return true;
}

void pgsql_shutdown(void){
	DEBUG_ASSERT(pgsql_conn != NULL);
	PQfinish(pgsql_conn);
	pgsql_conn = NULL;
}

static
void pgsql_get_date(struct db_date *dst, char *src){
	sscanf(src, "%d-%d-%d", &dst->year, &dst->month, &dst->day);
}

bool pgsql_load_account(const char *accname, struct db_result_account *acc){
	const char *param_values[] = { accname };
	PGresult *res = PQexecParams(pgsql_conn,
		"SELECT premend, password, charlist"
		" FROM accounts WHERE name = $1",
		1, NULL, param_values, NULL, NULL, 0);
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
		return false;
	DEBUG_ASSERT(PQnfields(res) == 3); // param count check
	DEBUG_ASSERT(PQntuples(res) == 1); // unique name account
	pgsql_get_date(&acc->premend, PQgetvalue(res, 0, 0));
	kpl_strncpy(acc->password, sizeof(acc->password), PQgetvalue(res, 0, 1));
	kpl_strncpy(acc->charlist, sizeof(acc->charlist), PQgetvalue(res, 0, 2));
	return true;
}

/*
bool pgsql_load_player(const char *name, struct db_result_player *player){
	const char *param_values[] = { name };
	PGresult *res;
	PQsendQueryParams(pgsql_conn,
		"SELECT *"
		" FROM players"
		" WHERE name = $1",
		1, NULL, param_values, NULL, NULL, 0);
	res = PQgetResult(pgsql_conn);
	if(res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
		return false;
}
*/

