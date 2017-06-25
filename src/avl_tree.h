#ifndef LIST_H_
#define LIST_H_

#include "mmblock.h"
#include <algorithm>

namespace kp{

template<typename T, int N>
class avl_tree{
private:
	struct node{
		T key;
		int height;
		node *parent;
		node *left;
		node *right;

		// node comparisson
		bool operator<(const node &rhs){ return (key < rhs.key); }
		bool operator>(const node &rhs){ return (key > rhs.key); }
	};

	node *root;
	int count;

	kp::mmblock<node, N> blk;

	// fix node height
	static void fix_height(node *x){
		int height = 0;
		if(x->right != nullptr)
			height = x->right->height;
		if(x->left != nullptr && x->left->height > height)
			height = x->left->height;
		x->height = height + 1;
	}

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

		// check if node needs a double rotation
		int balance = balance_factor(b);
		if(balance >= 1)
			b = rotate_left(b, b->right);
		else if(balance <= -1)
			b = rotate_right(b, b->left);

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

		// check if node needs a double rotation
		int balance = balance_factor(c);
		if(balance >= 1)
			c = rotate_left(c, c->right);
		else if(balance <= -1)
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

		x = x->parent;
		while(x != nullptr){
			fix_height(x);
			balance = balance_factor(x);
			if(balance >= 2)
				rotate_left(x, x->right);
			else if(balance <= -2)
				rotate_right(x, x->left);

			if(x->parent == nullptr)
				break;
			x = x->parent;
		}
		return x;
	}

public:
	// delete copy and move operations
	avl_tree(const avl_tree&) = delete;
	avl_tree &operator=(const avl_tree&) = delete;
	avl_tree(avl_tree&&) = delete;
	avl_tree &operator=(avl_tree&&) = delete;

	avl_tree(void) : root(nullptr), count(0), blk() {}
	~avl_tree(void){}

	template<typename G>
	T *insert(G &&value){
		// allocate node
		node *x = blk.alloc();
		if(x == nullptr){
			LOG_ERROR("avl_tree::insert: reached maximum capacity (%d)", N);
			return nullptr;
		}

		// initialize node values
		x->key = std::forward<G>(value);
		x->height = 1;
		x->parent = nullptr;
		x->left = nullptr;
		x->right = nullptr;

		// insert node into tree
		if(root == nullptr){
			root = x;
		}
		else{
			node *y = root;
			while(1){
				if(*x < *y){
					if(y->left == nullptr){
						y->left = x;
						break;
					}
					y = y->left;
				}
				else{
					if(y->right == nullptr){
						y->right = x;
						break;
					}
					y = y->right;
				}
			}
			x->parent = y;
			root = retrace(x);
		}

		return &x->key;
	}

	void remove(const T *value){
		node **it;
		// check if key is still valid
	}
};

} //namespace

#endif //LIST_H_
