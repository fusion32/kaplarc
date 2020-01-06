#ifndef RBTREE_H_
#define RBTREE_H_

#include "def.h"

#define RBTREE_CACHED_MIN 1

struct rbnode{
	int color;
	struct rbnode *parent;
	struct rbnode *left;
	struct rbnode *right;
};
#define RBNODE(n) ((struct rbnode*)(n))

typedef int (*rbt_compare_func)(struct rbnode*, struct rbnode*);
struct rbtree{
	struct rbnode	*root;
#ifdef RBTREE_CACHED_MIN
	struct rbnode	*cached_min;
#endif
	rbt_compare_func compare;
};
#define RBT_INITIALIZER(cmp) (struct rbtree){.root = NULL, .cached_min = NULL, .compare = cmp}

// init
void rbt_init(struct rbtree *t, rbt_compare_func compare);

// tree operations
struct rbnode *rbt_find(struct rbtree *t, struct rbnode *n);
bool rbt_insert(struct rbtree *t, struct rbnode *n);
void rbt_remove(struct rbtree *t, struct rbnode *n);

// tree traversal
bool rbt_empty(struct rbtree *t);
struct rbnode *rbt_min(struct rbtree *t);
struct rbnode *rbt_max(struct rbtree *t);
struct rbnode *rbnode_next(struct rbnode *cur);
struct rbnode *rbnode_prev(struct rbnode *cur);

#endif //RBTREE_H_
