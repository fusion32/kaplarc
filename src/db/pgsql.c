#include "db.h"

#if 0
//#ifdef DB_PGSQL

#include "../log.h"
#include <libpq-fe.h>

/*
Connection Parameters:
	{"host",		"localhost"},
	{"port",		"5432"},
	{"dbname",		"kaplar"},
	{"user",		"admin"},
	{"password",		"admin"},
	{"connect_timeout",	"5"},
	{"client_encoding",	"UTF8"},
	{"application_name",	"kaplar_server"},
	{"sslmode",		"disable"}

	// TODO: investigate how these SSL certificates and files works
	{"sslmode",		"prefer"},
	{"sslcert",		"~/.postgresql/postgresql.crt"},
	{"sslkey",		"~/.postgresql/postgresql.key"},
	{"sslrootcert",		"~/.postgresql/root.crt"},
	{"sslcrl",		"~/.postgresql/root.crl"},
*/

static PGconn *conn = NULL;

bool db_init(void){
	const char *k[] = {"host", "port", "dbname", "user", "password", "client_encoding"};
	const char *v[] = {"localhost", "5432", "kaplar", "admin", "admin", "UTF8"};

	if(conn != NULL)
		return true;

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

void db_shutdown(void){
	if(conn == NULL)
		return;
	PQfinish(conn);
	conn = NULL;
}

void db_test(void){
	//
}

#endif //DB_PGSQL
