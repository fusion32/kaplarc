#include "cassandra.h"
#include "../log.h"

static CassCluster *cluster = nullptr;
static CassSession *session = nullptr;

bool cass_init(void){
	CassFuture *future;
	CassError res;

	if(session != nullptr)
		return true;
	if(cluster != nullptr)
		cass_cluster_free(cluster);

	// create cluster handle
	cluster = cass_cluster_new();
	if(cluster == nullptr){
		LOG_ERROR("cass_init: failed to create cluster");
		return false;
	}

	// create session handle
	session = cass_session_new();
	if(session == nullptr){
		LOG_ERROR("cass_init: failed to create session");
		goto err0;
	}

	// set contact points
	res = cass_cluster_set_contact_points(cluster, "127.0.0.1");
	if(res != CASS_OK){
		// get result error string
		LOG_ERROR("cass_init: failed to set cluster contact points");
		goto err1;
	}

	// connect to cluster
	future = cass_session_connect(session, cluster);
	res = cass_future_error_code(future);
	if(res != CASS_OK){
		// get result error string
		LOG_ERROR("cass_init: failed to connect to cluster");
		goto err2;
	}

	// release connection future before returning
	cass_future_free(future);
	return true;

err2:	cass_future_free(future);
err1:	cass_session_free(session);
	session = nullptr;
err0:	cass_cluster_free(cluster);
	cluster = nullptr;
	return false;
}

void cass_shutdown(void){
	if(session != nullptr){
		cass_session_free(session);
		session = nullptr;
	}
	if(cluster != nullptr){
		cass_cluster_free(cluster);
		cluster = nullptr;
	}
}

bool row_get_column_string(const CassRow *row, const char *name, const char **output, size_t *output_size){
	const CassValue *value = cass_row_get_column_by_name(row, name);
	if(value == nullptr)
		return false;
	cass_value_get_string(value, output, output_size);
	return true;
}

void cass_test(void){
	const CassResult *result;
	const CassValue *value;
	const CassRow *row;
	CassStatement *stmt;
	CassFuture *future;
	const char *version;
	size_t version_len;

	if(session == nullptr)
		return;

	// prepare query
	stmt = cass_statement_new("SELECT release_version FROM system.local", 0);
	if(stmt == nullptr){
		LOG_ERROR("cass_test: failed to create statement");
		return;
	}

	// execute query
	future = cass_session_execute(session, stmt);
	result = cass_future_get_result(future);
	cass_future_free(future);
	if(result == nullptr){
		LOG_ERROR("cass_test: failed to execute statement");
		return;
	}

	// retrieve result
	row = cass_result_first_row(result);
	if(row == nullptr){
		LOG_ERROR("cass_test: 0 results");
		cass_result_free(result);
		return;
	}

	// get value
	//cass_row_get_column_by_name(row, "release_version");
	value = cass_row_get_column(row, 0);
	cass_value_get_string(value, &version, &version_len);
	if(row_get_column_string(row, "release_version", &version, &version_len))
		LOG("cass_test: release_version = %.*s", version_len, version);
	else
		LOG_ERROR("cass_test: failed to convert `release_version` to string");

	cass_result_free(result);
}
