#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "dispatcher.h"

#include "net/net.h"
#include "server/server.h"
#include "server/protocol_test.h"

#include <stdlib.h>
#include <vector>


int main(int argc, char **argv){
	LOG("Initializing dispatcher queues...");
	Dispatcher *main_dispatcher;
	dispatcher_create(&main_dispatcher, 0);

	LOG("Initializing scheduler...");
	scheduler_init(main_dispatcher);
	atexit(scheduler_shutdown);

	LOG("Initializing network interface...");
	if(!net_init())
		return -1;
	atexit(net_shutdown);


	server_add_protocol<ProtocolTest>(7171);
	server_run();
	return 0;
}
