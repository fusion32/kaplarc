#include <vector>

#include "log.h"
#include "scheduler.h"
#include "system.h"
#include "work.h"

#include "netlib/network.h"
#include "net/server.h"
#include "net/protocol_test.h"

/*
int main(int argc, char **argv){
	work_init();
	scheduler_init();
	net_init();

	server_add_protocol<ProtocolTest>(7171);
	server_run();

	net_shutdown();
	scheduler_shutdown();
	work_shutdown();
	return 0;
}
*/

#include "avltree.h"
int main(int argc, char **argv){
	AVLTree<int, 256> tree;
	for(int i = 0; i < 256; ++i)
		tree.insert((i * 1337) % 256);

	LOG("1 - tree height = %d", tree.height());
	for(const int &num : tree)
		LOG("1 - %d", num);

	auto it = tree.find(16);//tree.lower_bound(16);
	while(it != tree.end())
		it = tree.erase(it);

	LOG("2 - tree height = %d", tree.height());
	for(const int &num : tree)
		LOG("2 - %d", num);

	getchar();
	return 0;
}
