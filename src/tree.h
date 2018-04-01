// C compatible source: don't change if you are
// going to break compatibility

#ifndef TREE_H_
#define TREE_H_

#include "def.h"

struct tree_node{
	struct tree_node *parent;
	struct tree_node *left;
	struct tree_node *right;
	uintptr data;
	char key[];
};

// int r = tree->cmp(a, b);
// r should be:
//	r < 0, if `a` is less than `b`
//	r > 0, if `a` is greater than `b`
//	r = 0, if `a` is equal to `b`
struct tree{
	long keysize;
	struct tree_node *root;
	struct memblock *blk;
	int (*cmp)(void*, void*);
};

// tree interface
struct tree *tree_create(long slots, long keysize, int(*cmp)(void*, void*));
void tree_destroy(struct tree *t);
int tree_empty(struct tree *t);
struct tree_node *tree_min(struct tree *t);
struct tree_node *tree_max(struct tree *t);
struct tree_node *tree_find(struct tree *t, void *key);
struct tree_node *tree_insert(struct tree *t, void *key);
struct tree_node *tree_erase(struct tree *t, struct tree_node *n);
void tree_remove(struct tree *t, struct tree_node *n);

// tree utility
struct tree_node *tree_alloc_node(struct tree *t);
void tree_free_node(struct tree *t, struct tree_node *n);
void tree_insert_node(struct tree *t, struct tree_node *n);
void tree_remove_node(struct tree *t, struct tree_node *n, struct tree_node **plowest);
struct tree_node *tree_find_node(struct tree *t, void *key);

// node utility
void *tree_node_key(struct tree_node *n);
struct tree_node *tree_node_next(struct tree_node *cur);
struct tree_node *tree_node_prev(struct tree_node *cur);
struct tree_node *tree_node_rotate_left(struct tree_node *a);
struct tree_node *tree_node_rotate_right(struct tree_node *a);


#endif //TREE_H_
