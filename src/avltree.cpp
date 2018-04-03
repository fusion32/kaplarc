// C compatible source: don't change if you are
// going to break compatibility

#include "avltree.h"
#include "tree.h"
#include "def.h"
#include "log.h"

#include <string.h>

static void fix_height(struct tree_node *n){
	int height = 0;
	if(n->right != nullptr)
		height = n->right->data;
	if(n->left != nullptr && n->left->data > height)
		height = n->left->data;
	n->data = height + 1;
}

static int balance_factor(struct tree_node *n){
	int balance = 0;
	if(n->right != nullptr)
		balance += n->right->data;
	if(n->left != nullptr)
		balance -= n->left->data;
	return balance;
}

// avl tree interface
struct tree_node *avl_insert(struct tree *t, void *key){
	struct tree_node *n = tree_alloc_node(t);
	if(n != NULL){
		memcpy(n->key, key, t->keysize);
		avl_insert_node(t, n);
	}
	return n;
}

struct tree_node *avl_erase(struct tree *t, struct tree_node *n){
	struct tree_node *next = NULL;
	if(n != NULL){
		next = tree_node_next(n);
		avl_remove_node(t, n);
		tree_free_node(t, n);
	}
	return next;
}

void avl_remove(struct tree *t, struct tree_node *n){
	if(n != NULL){
		avl_remove_node(t, n);
		tree_free_node(t, n);
	}
}

int avl_height(struct tree *t){
	if(t->root != NULL)
		return t->root->data;
	return 0;
}

// avl tree utility
void avl_insert_node(struct tree *t, struct tree_node *n){
	if(t->root == NULL){
		t->root = n;
	}else{
		tree_insert_node(t, n);
		avl_retrace(t, n);
	}
}

void avl_remove_node(struct tree *t, struct tree_node *n){
	struct tree_node *retrace_from;
	tree_remove_node(t, n, &retrace_from);
	if(retrace_from != NULL)
		avl_retrace(t, retrace_from);
}

void avl_retrace(struct tree *t, struct tree_node *n){
	struct tree_node *p;
	int b;
	while(1){
		// save parent for next step
		p = n->parent;

		// rebalance subtree
		fix_height(n);
		b = balance_factor(n);
		if(b >= 2){
			if(balance_factor(n->right) <= -1)
				avl_node_rotate_right(n->right);
			n = avl_node_rotate_left(n);
		}else if(b <= -2){
			if(balance_factor(n->left) >= 1)
				avl_node_rotate_left(n->left);
			n = avl_node_rotate_right(n);
		}

		// check if the root was reached
		if(p == NULL){
			t->root = n;
			return;
		}

		// climb to parent
		n = p;
	}
}

// avl tree node specific utility
struct tree_node *avl_node_rotate_left(struct tree_node *a){
	struct tree_node *c = tree_node_rotate_left(a);
	// fix heights after rotation
	fix_height(a);
	fix_height(c);
	if(c->parent != NULL)
		fix_height(c->parent);
	return c;
}

struct tree_node *avl_node_rotate_right(struct tree_node *a){
	struct tree_node *b = tree_node_rotate_right(a);
	// fix heights after rotation
	fix_height(a);
	fix_height(b);
	if(b->parent != NULL)
		fix_height(b->parent);
	return b;
}
