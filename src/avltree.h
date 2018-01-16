#ifndef AVLTREE_H_
#define AVLTREE_H_

#include "memblock.h"

template<typename T, int N>
class AVLTree{
private:
	struct node{
		T key;
		int height;
		node *parent;
		node *left;
		node *right;
	};

	// tree root
	node *root;

	// tree memory
	MemBlock<node, N> blk;

	// fix node height
	static void fix_height(node *x){
		int height = 0;
		if(x->right != nullptr)
			height = x->right->height;
		if(x->left != nullptr && x->left->height > height)
			height = x->left->height;
		x->height = height + 1;
	}

	// calc node balance factor
	static int balance_factor(node *x){
		int balance = 0;
		if(x->right != nullptr)
			balance += x->right->height;
		if(x->left != nullptr)
			balance -= x->left->height;
		return balance;
	}

	//
	//   (parent)    (parent)
	//      |           |
	//     (a)   =>    (b)
	//     / \   =>    / \ 
	//   (b)  c  =>   d  (a)
	//   / \     =>      / \
	//  d  (e)         (e)  c
	//
	static node *rotate_right(node *a, node *b){
		//assert(a->left == b);

		// check if node needs a left_right rotation
		if(balance_factor(b) >= 1)
			b = rotate_left(b, b->right);

		b->parent = a->parent;
		a->parent = b;
		if(b->parent != nullptr){
			if(b->parent->left == a)
				b->parent->left = b;
			else
				b->parent->right = b;
		}

		a->left = b->right;
		b->right = a;
		if(a->left != nullptr)
			a->left->parent = a;

		// fix (a), (b), and (parent) heights
		fix_height(a);
		fix_height(b);
		if(b->parent != nullptr)
			fix_height(b->parent);

		return b;
	}

	//
	// (parent)       (parent)
	//    |              |
	//   (a)     =>     (c)
	//   / \     =>     / \
	//  b  (c)   =>   (a)  e
	//     / \   =>   / \
	//   (d)  e      b  (d)
	//
	static node *rotate_left(node *a, node *c){
		//assert(a->right == c);

		// check if node needs a right_left rotation
		if(balance_factor(c) <= -1)
			c = rotate_right(c, c->left);

		c->parent = a->parent;
		a->parent = c;
		if(c->parent != nullptr){
			if(c->parent->left == a)
				c->parent->left = c;
			else
				c->parent->right = c;
		}

		a->right = c->left;
		c->left = a;
		if(a->right != nullptr)
			a->right->parent = a;

		// fix (a), (c), and (parent) heights
		fix_height(a);
		fix_height(c);
		if(c->parent != nullptr)
			fix_height(c->parent);

		return c;
	}

	// retrace from node `x` (this is assuming `x` has the correct height)
	// returns the new root node (may not have changed)
	static node *retrace(node *x){
		int balance;

		while(x->parent != nullptr){
			x = x->parent;
			fix_height(x);
			balance = balance_factor(x);
			if(balance >= 2)
				x = rotate_left(x, x->right);
			else if(balance <= -2)
				x = rotate_right(x, x->left);
		}
		return x;
	}

	// insert node into the tree
	void insert_node(node *x){
		if(root != nullptr){
			node **y = &root;
			while(*y != nullptr){
				x->parent = (*y);
				if(x->key < (*y)->key)
					y = &(*y)->left;
				else
					y = &(*y)->right;
			}
			*y = x;
			root = retrace(x);
		} else {
			root = x;
		}
	}

	// `G` is anything that can be compared to `T`
	template<typename G>
	node *find_node(const G &value){
		node *x = root;
		while(x != nullptr && x->key != value){
			if(x->key > value)
				x = x->left;
			else
				x = x->right;
		}
		return x;
	}

public:
	// delete copy and move operations
	AVLTree(const AVLTree&)			= delete;
	AVLTree(AVLTree&&)			= delete;
	AVLTree &operator=(const AVLTree&)	= delete;
	AVLTree &operator=(AVLTree&&)		= delete;

	AVLTree(void) : root(nullptr), blk() {}
	~AVLTree(void){}

	template<typename G>
	T *insert(G &&value){
		// allocate node
		node *x = blk.alloc();
		if(x == nullptr)
			return nullptr;

		// initialize node
		x->key = std::forward<G>(value);
		x->height = 1;
		x->parent = nullptr;
		x->left = nullptr;
		x->right = nullptr;

		// insert into the tree
		insert_node(x);
		return &x->key;
	}

	template <typename... Args>
	T *emplace(Args&&... args){
		// allocate node
		node *x = blk.alloc();
		if(x == nullptr)
			return nullptr;

		// initialize node
		new(&x->key) T(std::forward<Args>(args)...);
		x->height = 1;
		x->parent = nullptr;
		x->left = nullptr;
		x->right = nullptr;

		// insert into the tree
		insert_node(x);
		return &x->key;
	}


	template<typename G>
	bool remove(const G &value){
		node *y = nullptr;
		node *x = find_node(value);
		if(x == nullptr)
			return false;

		if(x->left != nullptr && x->right != nullptr){
			y = x->right;
			while(y->left != nullptr)
				y = y->left;

			// pull `y->right` subtree
			y->parent->left = y->right;
			if(y->right != nullptr)
				y->right->parent = y->parent;

			// replace `x` with `y`
			y->left = x->left;
			y->left->parent = y;
			y->right = x->right;
			y->right->parent = y;
		} else if(x->left != nullptr) {
			y = x->left;
		} else if(x->right != nullptr) {
			y = x->right;
		}

		// fix parent link
		if(y != nullptr)
			y->parent = x->parent;
		if(x->parent != nullptr){
			if(x->parent->left == x)
				x->parent->left = y;
			else
				x->parent->right = y;
		}

		// check if root needs to be updated
		if(root == x)
			root = y;

		// free node
		x->key.~T();
		blk.free(x);
		return true;
	}

	bool empty(void){
		return (root == nullptr);
	}

	template<typename G>
	bool contains(const G &value){
		return (find_node(value) != nullptr);
	}

	template<typename G>
	T *find(const G &value){
		node *x = find_node(value);
		if(x == nullptr)
			return nullptr;
		return &x->key;
	}

	T *min(void){
		if(root == nullptr)
			return nullptr;

		node *x = root;
		while(x->left != nullptr)
			x = x->left;
		return &x->key;
	}

	T *max(void){
		if(root == nullptr)
			return nullptr;

		node *x = root;
		while(x->right != nullptr)
			x = x->right;
		return &x->key;
	}
};

#endif //AVLTREE_H_
