#include "DATABASE.h"

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

bool database_init(void){
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

void database_shutdown(void){
	DEBUG_ASSERT(conn != NULL);
	PQfinish(conn);
	conn = NULL;
}

void database_test(void){
	PGresult *res;
	int rows;

	res = PQexec(conn, "SELECT name, password, premend, charlist FROM accounts");
	if(res == NULL){
		LOG_ERROR("pgsql_test: failed to create result structure");
		return;
	}

	if(PQresultStatus(res) != PGRES_TUPLES_OK){
		LOG_ERROR("pgsql_test: select failed:\n%s", PQresultErrorMessage(res));
		PQclear(res);
		return;
	}

	rows = PQntuples(res);
	DEBUG_ASSERT(PQnfields(res) == 4);
	for(int i = 0; i < rows; i += 1){
		LOG("'%s', '%s', '%s', '%s'",
			PQgetvalue(res, i, 0),
			PQgetvalue(res, i, 1),
			PQgetvalue(res, i, 2),
			PQgetvalue(res, i, 3));
	}
	PQclear(res);
}
