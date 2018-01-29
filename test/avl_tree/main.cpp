#include "../../src/avltree.h"
#include "../../src/log.h"

int main(int argc, char **argv){
	AVLTree<int, 256> tree;
	for(int i = 0; i < 256; ++i)
		tree.insert((i * 1337) % 256);

	LOG("1 - tree height = %d", tree.height());
	for(const int &num : tree)
		LOG("1 - %d", num);

	auto it = tree.find(16);
	while(it != tree.end())
		it = tree.erase(it);

	LOG("2 - tree height = %d", tree.height());
	for(const int &num : tree)
		LOG("2 - %d", num);

	getchar();
	return 0;
}
