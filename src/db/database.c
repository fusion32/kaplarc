#define DB_INTERNAL 1
#include "database.h"
#include "../thread.h"

// thread sync
static bool running;
static thread_t thr;
static mutex_t mtx;
static condvar_t cv;

// loads
struct load_request{
	uint32 load_op;
	db_load_key_t key;
	db_load_callback_t callback;
	void *udata;
};
#define MAX_QUEUED_LOADS 1024
#if !IS_POWER_OF_TWO(MAX_QUEUED_LOADS)
#	error "MAX_QUEUED_LOADS must be a power of two."
#endif
#define LOADS_BITMASK (MAX_QUEUED_LOADS - 1)
static struct load_request loads[MAX_QUEUED_LOADS];
static uint32 loads_readpos = 0;
static uint32 loads_writepos = 0;

static INLINE bool loads_empty(void){
	return loads_readpos == loads_writepos;
}

// stores
// @TODO


static void *db_thread(void *unused){
	while(1){
		mutex_lock(&mtx);
		// check if still running
		if(!running){
			mutex_unlock(&mtx);
			break;
		}
		// @TODO: pop task from ringbuffer
		mutex_unlock(&mtx);
		// @TODO: execute task
	}
	return NULL;
}

bool db_init(void){
	// connect to database
	if(!db_internal_connect()){
		LOG_ERROR("db_init: failed to initialize database connection");
		return false;
	}
	// create db thread
	running = true;
	mutex_init(&mtx);
	if(thread_create(&thr, db_thread, NULL) != 0){
		mutex_destroy(&mtx);
		db_internal_connection_close();
		return false;
	}
	return true;
}

void db_shutdown(void){
	// join database thread
	mutex_lock(&mtx);
	running = false;
	mutex_unlock(&mtx);
	thread_join(&thr, NULL);

	// close database connection
	db_internal_connection_close();
}

