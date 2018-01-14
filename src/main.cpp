#include <vector>

#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "work.h"

#include "netlib/network.h"
#include "net/server.h"
#include "net/protocol_test.h"

int main(int argc, char **argv){
	work_init();
	scheduler_init();
	net_init();

	Server::instance()->add_protocol<ProtocolTest>(7171);
	Server::instance()->run();

	net_shutdown();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}
