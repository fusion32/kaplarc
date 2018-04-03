// C compatible source: don't change if you are
// going to break compatibility

#include "tree.h"

#include "log.h"
#include "memblock.h"

#include <stdlib.h>
#include <string.h>

// tree interface
struct tree *tree_create(long slots, long keysize, int(*cmp)(void*, void*)){
	struct tree *t;
	struct memblock *blk;

	if(cmp == NULL){
		LOG_ERROR("tree_create: `cmp` must be a valid comparison function");
		return NULL;
	}

	// create memory block
	blk = memblock_create(slots, sizeof(struct tree_node) + keysize);
	if(blk == NULL){
		LOG_ERROR("tree_create: failed to create memory block");
		return NULL;
	}

	// create control block
	t = (struct tree*)malloc(sizeof(struct tree));
	if(t == NULL){
		memblock_destroy(blk);
		LOG_ERROR("tree_create: out of memory");
		return NULL;
	}
	t->keysize = keysize;
	t->root = NULL;
	t->blk = blk;
	t->cmp = cmp;
	return t;
}

void tree_destroy(struct tree *t){
	if(t == NULL)
		return;

	// release memory block
	if(t->blk != NULL){
		memblock_destroy(t->blk);
		t->blk = NULL;
	}

	// release control block
	free(t);
}

int tree_empty(struct tree *t){
	return (t->root == NULL);
}

struct tree_node *tree_min(struct tree *t){
	struct tree_node *cur = t->root;
	if(cur != NULL){
		while(cur->left != NULL)
			cur = cur->left;
	}
	return cur;
}

struct tree_node *tree_max(struct tree *t){
	struct tree_node *cur = t->root;
	if(cur != NULL){
		while(cur->right != NULL)
			cur = cur->right;
	}
	return cur;
}

struct tree_node *tree_find(struct tree *t, void *key){
	return tree_find_node(t, key);
}

struct tree_node *tree_insert(struct tree *t, void *key){
	struct tree_node *n = tree_alloc_node(t);
	if(n != NULL){
		memcpy(n->key, key, t->keysize);
		tree_insert_node(t, n);
	}
	return n;
}

struct tree_node *tree_erase(struct tree *t, struct tree_node *n){
	struct tree_node *next = NULL;
	if(n != NULL){
		next = tree_node_next(n);
		tree_remove_node(t, n, NULL);
		tree_free_node(t, n);
	}
	return next;
}

void tree_remove(struct tree *t, struct tree_node *n){
	if(n != NULL){
		tree_remove_node(t, n, NULL);
		tree_free_node(t, n);
	}
}

// tree utility
struct tree_node *tree_alloc_node(struct tree *t){
	return (struct tree_node*)memblock_alloc(t->blk);
}

void tree_free_node(struct tree *t, struct tree_node *n){
	memblock_free(t->blk, n);
}

void tree_insert_node(struct tree *t, struct tree_node *n){
	if(t->root != NULL){
		struct tree_node **y = &t->root;
		while(*y != NULL){
			n->parent = *y;
			if(t->cmp(n->key, (*y)->key) < 0)
				y = &(*y)->left;
			else
				y = &(*y)->right;
		}
		*y = n;
	}else{
		t->root = n;
	}
}

void tree_remove_node(struct tree *t, struct tree_node *n, struct tree_node **plowest){
	struct tree_node *y = NULL;
	struct tree_node *lowest = NULL;
	if(n->left != NULL && n->right != NULL){
		// find smallest element y on the n->right subtree
		y = n->right;
		while(y->left != NULL)
			y = y->left;

		// if y is n->right, it will be the lowest modified node
		if(y == n->right)
			lowest = y;
		// else the lowest modified node will be y->parent
		else
			lowest = y->parent;

		// pop y from the tree
		if(y->right != NULL)
			y->right->parent = y->parent;
		if(y->parent->left == y)
			y->parent->left = y->right;
		else
			y->parent->right = y->right;
		// replace n with y
		y->left = n->left;
		y->left->parent = y;
		y->right = n->right;
		// n->right may have become null from the y pop
		if(y->right) y->right->parent = y;
	}else if(n->left != NULL){
		y = n->left;
		lowest = y;
	}else if(n->right != NULL){
		y = n->right;
		lowest = y;
	}else{
		//y = NULL;
		lowest = n->parent;
	}

	// fix parent link
	if(y != NULL)
		y->parent = n->parent;
	if(n->parent){
		if(n->parent->left == n)
			n->parent->left = y;
		else
			n->parent->right = y;
	}

	if(lowest == NULL)
		t->root = NULL;

	if(plowest != NULL)
		*plowest = lowest;
}

struct tree_node *tree_find_node(struct tree *t, void *key){
	struct tree_node *cur = t->root;
	while(cur != NULL){
		int r = t->cmp(key, cur->key);
		if(r < 0)
			cur = cur->left;
		else if(r > 0)
			cur = cur->right;
		else
			return cur;
	}
	return NULL;
}

// node utility
void *tree_node_key(struct tree_node *n){
	if(n == NULL) return NULL;
	return n->key;
}

struct tree_node *tree_node_next(struct tree_node *cur){
	if(cur == NULL)
		return NULL;

	if(cur->right != NULL){
		// elements from the right subtree are all
		// greater than the current node so we need
		// to find the leftmost(smallest) element
		// from that subtree
		cur = cur->right;
		while(cur->left != NULL)
			cur = cur->left;
	}else{
		// if there is no right subtree, we need to
		// iterate back to a parent greater than its
		// previous element
		while(cur->parent && cur->parent->left != cur)
			cur = cur->parent;
		if(cur->parent)
			cur = cur->parent;
		else
			cur = nullptr;
	}
	return cur;
}

struct tree_node *tree_node_prev(struct tree_node *cur){
	// wrap around behaviour
	if(cur == NULL)
		return NULL;

	if(cur->left != nullptr){
		// same as next_node but in reverse
		cur = cur->left;
		while(cur->right != nullptr)
			cur = cur->right;
	}else{
		// same as next_node but in reverse
		while(cur->parent && cur->parent->right != cur)
			cur = cur->parent;
		if(cur->parent)
			cur = cur->parent;
		else
			cur = nullptr;
	}
	return cur;
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
struct tree_node *tree_node_rotate_left(struct tree_node *a){
	struct tree_node *c = a->right;
	if(a->parent != NULL){
		if(a->parent->left == a)
			a->parent->left = c;
		else
			a->parent->right = c;
	}
	c->parent = a->parent;
	a->parent = c;
	a->right = c->left;
	c->left = a;
	if(a->right != NULL)
		a->right->parent = a;
	return c;
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
struct tree_node *tree_node_rotate_right(struct tree_node *a){
	struct tree_node *b = a->left;
	if(a->parent != NULL){
		if(a->parent->left == a)
			a->parent->left = b;
		else
			a->parent->right = b;
	}
	b->parent = a->parent;
	a->parent = b;
	a->left = b->right;
	b->right = a;
	if(a->left != NULL)
		a->left->parent = a;
	return b;
}
