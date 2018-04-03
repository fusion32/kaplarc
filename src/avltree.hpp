#ifndef AVLTREE_HPP_
#define AVLTREE_HPP_

#include "avltree.h"
#include "tree.hpp"

template<typename T, typename Compare = std::less<T>>
class AVLTree : public Tree<T, Compare> {
public:
	AVLTree(void) {}
	~AVLTree(void) {}

	template<typename... Args>
	TreeIterator<T> insert(Args&&... args){
		// allocate node
		struct tree_node *node = tree_alloc_node(self);
		if(node == nullptr)
			return nullptr;

		// initialize node
		node->parent = nullptr;
		node->left = nullptr;
		node->right = nullptr;
		node->data = 1; // node height
		construct_key(node->key, std::forward<Args>(args)...);

		// insert node
		avl_insert_node(self, node);
		return node;
	}

	TreeIterator<T> erase(const TreeIterator<T> &it){
		auto next = it + 1;
		remove(it);
		return next;
	}

	void remove(const TreeIterator<T> &it){
		destruct_key(it);
		avl_remove(self, it.node());
	}

	int height(void){
		return avl_height(self);
	}
};

#endif //AVLTREE_HPP_
