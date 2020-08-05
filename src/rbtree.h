#ifndef KAPLAR_RBTREE_H_
#define KAPLAR_RBTREE_H_ 1

#include "common.h"

#define RBTREE_CACHED_MIN 1

// some helpers
#define RBNODE(n)			((struct rbnode*)(n))
#define RBT_INSERT(t, n)		rbt_insert((t), RBNODE(n))
#define RBT_REMOVE(t, n)		rbt_remove((t), RBNODE(n))
#define RBT_MIN(node_type, t)		((node_type*)rbt_min((t)))
#define RBT_FIND(node_type, t, n)	((node_type*)rbt_find((t), RBNODE(n)))
#define RBT_INITIALIZER(cmp)		(struct rbtree){NULL, NULL, cmp}

#define RBNODE_RED 0
#define RBNODE_BLACK 1

struct rbnode{
	int color;
	struct rbnode *parent;
	struct rbnode *left;
	struct rbnode *right;
};

typedef int (*rbt_compare_func)(struct rbnode*, struct rbnode*);
struct rbtree{
	struct rbnode	*root;
#ifdef RBTREE_CACHED_MIN
	struct rbnode	*cached_min;
#endif
	rbt_compare_func compare;
};

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

#endif //KAPLAR_RBTREE_H_
