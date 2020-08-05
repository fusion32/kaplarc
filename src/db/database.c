#define DB_INTERNAL 1

#include "database.h"



static void *db_thread(void *unused){

	return NULL;
}

bool db_init(void){
	if(!db_internal_connect()){
		LOG_ERROR("db_init: failed to initialize database connection");
		return false;
	}
	// create db thread
	return true;
}

void db_shutdown(void){
	db_internal_connection_close();
}

