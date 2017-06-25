#include "../../src/avl_tree.h"

//
// hack to retrieve the private tree root
//
template<typename T>
struct node{
	T key;
	int height;
	node *parent;
	node *left;
	node *right;
};

template<typename T>
struct avl_tree_struct{
	node<T> *root;
};

template<typename T>
static void print_node(node<T> *n)
{
	if(n == nullptr) return;
	if(n->height > 2){
		print_node(n->left);
		print_node(n->right);
	}
	else{
		if(n->left != nullptr)	LOG("%d", n->left->key);
					LOG("%d", n->key);
		if(n->right != nullptr)	LOG("%d", n->right->key);
	}
}

int main(int argc, char **argv)
{
	kp::avl_tree<int, 256> tree;
	auto ptr = (avl_tree_struct<int>*)&tree;
	for(int i = 0; i < 256; ++i)
		tree.insert((i*1337)%256);
	print_node(ptr->root);
	return 0;
}