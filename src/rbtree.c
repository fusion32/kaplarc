#include "rbtree.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

#define RED 0
#define BLACK 1

// init
void rbt_init(struct rbtree *t, rbt_compare_func compare){
	ASSERT(t != NULL && "rbt_init: NULL tree pointer");
	ASSERT(compare != NULL &&
		"rbt_init: invalid compare function");
	t->root = NULL;
#ifdef RBTREE_CACHED_MIN
	t->cached_min = NULL;
#endif
	t->compare = compare;
}


// tree operations
struct rbnode *rbt_find(struct rbtree *t, struct rbnode *n){
	struct rbnode *cur = t->root;
	int cmp;
	while(cur != NULL){
		cmp = t->compare(cur, n);
		if(cmp == 0)
			return cur;
		else if(cmp < 0)
			cur = cur->left;
		else
			cur = cur->right;
	}
	return cur;
}

// (parent)       (parent)
//    |              |
//   (a)     =>     (c)
//   / \     =>     / \
//  b  (c)   =>   (a)  e
//     / \   =>   / \
//   (d)  e      b  (d)
//
static void rbt_rotate_left(struct rbtree *t, struct rbnode *a){
	struct rbnode *c = a->right;
	if(a->parent != NULL){
		if(a->parent->left == a)
			a->parent->left = c;
		else
			a->parent->right = c;
	}else{
		t->root = c;
	}
	c->parent = a->parent;
	a->parent = c;
	a->right = c->left;
	c->left = a;
	if(a->right != NULL)
		a->right->parent = a;
}

//   (parent)    (parent)
//      |           |
//     (a)   =>    (b)
//     / \   =>    / \
//   (b)  c  =>   d  (a)
//   / \     =>      / \
//  d  (e)         (e)  c
//
static void rbt_rotate_right(struct rbtree *t, struct rbnode *a){
	struct rbnode *b = a->left;
	if(a->parent != NULL){
		if(a->parent->left == a)
			a->parent->left = b;
		else
			a->parent->right = b;
	}else{
		t->root = b;
	}
	b->parent = a->parent;
	a->parent = b;
	a->left = b->right;
	b->right = a;
	if(a->left != NULL)
		a->left->parent = a;
}

static INLINE void rbt_insert_fixup(struct rbtree *t, struct rbnode *node){
	struct rbnode *parent, *gparent, *uncle;

	// node is always RED inside the loop
	while(node->parent != NULL && node->parent->color == RED){
		// as root is always BLACK and `parent` is RED,
		// it cannot be root which means `gparent` != NULL
		parent = node->parent;
		gparent = parent->parent;
		if(gparent->left == parent){
			// parent is a left child
			uncle = gparent->right;
			if(uncle && uncle->color == RED){
				parent->color = BLACK;
				gparent->color = RED;
				uncle->color = BLACK;
				node = gparent;
			}else{
				if(parent->right == node){
					rbt_rotate_left(t, parent);
					rbt_rotate_right(t, gparent);
					node->color = BLACK;
					gparent->color = RED;
					// in this case `node` will take place
					// of `gparent` so we don't need to
					// update it for the next iteration
				}else{
					rbt_rotate_right(t, gparent);
					parent->color = BLACK;
					gparent->color = RED;
					// `parent` will take place of `gparent`
					// so we need to update `node` for the
					// next iteration
					node = parent;
				}
			}
		}else{
			// parent is a right child (same as above but inverted)
			uncle = gparent->left;
			if(uncle && uncle->color == RED){
				parent->color = BLACK;
				gparent->color = RED;
				uncle->color = BLACK;
				node = gparent;
			}else{
				if(parent->left == node){
					rbt_rotate_right(t, parent);
					rbt_rotate_left(t, gparent);
					node->color = BLACK;
					gparent->color = RED;
				}else{
					rbt_rotate_left(t, gparent);
					parent->color = BLACK;
					gparent->color = RED;
					node = parent;
				}
			}
		}
	}

	// tree root is always BLACK
	t->root->color = BLACK;
}

bool rbt_insert(struct rbtree *t, struct rbnode *n){
	struct rbnode **y;
	int cmp;

	n->color = RED;
	n->parent = NULL;
	n->left = NULL;
	n->right = NULL;
	if(t->root != NULL){
		y = &t->root;
		while(*y != NULL){
			n->parent = *y;
			cmp = t->compare(n, *y);
			if(cmp == 0)
				return false;
			else if(cmp < 0)
				y = &(*y)->left;
			else
				y = &(*y)->right;
		}
		*y = n;
	}else{
		t->root = n;
	}
#ifdef RBTREE_CACHED_MIN
	// this is after the insert because it may fail if
	// theres already an equal element
	if(t->cached_min == NULL ||
	  t->compare(n, t->cached_min) < 0)
		t->cached_min = n;
#endif
	rbt_insert_fixup(t, n);
	return true;
}

//
// this remove fixup is almost the same used in the LINUX kernel
//
// NOTES:
// - the node's subtree have a black height that is 1 lower than
// the other subtrees which implies that sibling cannot be NULL
// as it's subtree has at least one extra black node
//
// when node is red:
//	- it's the simplest case as we can set the node's color
//	to black and the subtree black height will be restored
//
// when node is black:
// case 1: sibling is red
//	- this is a setup for the other cases, and it's making
//	sure that the sibling is black while keeping the black
//	heights the same
// case 2: sibling children are black
//	- we can change the sibling color to red to decrease the
//	black height of its subtree to match the node's black height
//	- if parent is red, we can set it to black to restore the
//	original black height of the subtree
//	- if parent is black, we need to fixup again from it to
//	propagate the change in black height up the tree
// case 3: sright(sleft) is black and sleft(sright) is red
//	- this is a setup for case 4, and changes the subtree so
//	sright(sleft) is red while keeping sibling black
//	- IMPORTANT: the implementation doesn't actually change the
//	colors in this stage because they'll be overwritten in case 4
// case 4: sright(sleft) is red
//	- this will increase the black height in the node's subtree
//	effectively rebalancing the tree
//
static INLINE void rbt_remove_fixup(struct rbtree *t,
		struct rbnode *node, struct rbnode *parent){
	struct rbnode *sibling, *sleft, *sright;

	if(node != NULL && node->color == RED){
		node->color = BLACK;
		return;
	}

	if(parent == NULL)
		return;

	while(1){
		if(parent->left == node){
			sibling = parent->right;
			if(sibling->color == RED){
				// case 1
				sibling->color = BLACK;
				parent->color = RED;
				rbt_rotate_left(t, parent);
				sibling = parent->right;
			}

			sright = sibling->right;
			if(sright == NULL || sright->color == BLACK){
				sleft = sibling->left;
				if(sleft == NULL || sleft->color == BLACK){
					// case 2
					sibling->color = RED;
					if(parent->color == RED){
						parent->color = BLACK;
					}else{
						node = parent;
						parent = node->parent;
						if(parent != NULL)
							continue;
					}
					break;
				}

				// case 3
				rbt_rotate_right(t, sibling);
				sright = sibling;
				sibling = sleft;
			}

			// case 4
			sibling->color = parent->color;
			parent->color = BLACK;
			sright->color = BLACK;
			rbt_rotate_left(t, parent);
			break;
		}else{
			// same as above but inverted
			sibling = parent->left;
			if(sibling->color == RED){
				// case 1
				sibling->color = BLACK;
				parent->color = RED;
				rbt_rotate_right(t, parent);
				sibling = parent->left;
			}

			sleft = sibling->left;
			if(sleft == NULL || sleft->color == BLACK){
				sright = sibling->right;
				if(sright == NULL || sright->color == BLACK){
					// case 2
					sibling->color = RED;
					if(parent->color == RED){
						parent->color = BLACK;
					}else{
						node = parent;
						parent = node->parent;
						if(parent)
							continue;
					}
					break;
				}

				// case 3
				rbt_rotate_left(t, sibling);
				sleft = sibling;
				sibling = sright;
			}

			// case 4
			sibling->color = parent->color;
			parent->color = BLACK;
			sleft->color = BLACK;
			rbt_rotate_right(t, parent);
			break;
		}
	}
}

void rbt_remove(struct rbtree *t, struct rbnode *n){
	// we keep track of the rebalance parent, because the
	// rebalance node might be NULL
	struct rbnode *rebalance_node;
	struct rbnode *rebalance_parent;
	struct rbnode *y;
	int rm_color;

#ifdef RBTREE_CACHED_MIN
	if(t->cached_min == n)
		t->cached_min = rbnode_next(n);
#endif

	rm_color = n->color;
	if(n->left != NULL && n->right != NULL){
		y = n->right;
		while(y->left != NULL)
			y = y->left;

		// update removed color
		rm_color = y->color;
		y->color = n->color;

		// rebalance_node is always y->right but
		// rebalance_parent may be y itself if
		// y == n->right
		rebalance_node = y->right;
		if(y == n->right){
			rebalance_parent = y;

			// pull right subtree
			y->left = n->left;
			y->left->parent = y;
		}else{
			rebalance_parent = y->parent;

			// remove `y` from the tree
			y->parent->left = y->right;
			if(y->right != NULL)
				y->right->parent = y->parent;

			// insert `y` in place of `n`
			y->left = n->left;
			y->left->parent = y;
			y->right = n->right;
			y->right->parent = y;
		}
	}else{
		y = (n->left != NULL)
			? n->left
			: n->right;
		rebalance_node = y;
		rebalance_parent = n->parent;
	}

	if(y != NULL)
		y->parent = n->parent;
	if(n->parent != NULL){
		if(n->parent->left == n)
			n->parent->left = y;
		else
			n->parent->right = y;
	}else{
		t->root = y;
	}

	if(rm_color == BLACK)
		rbt_remove_fixup(t, rebalance_node, rebalance_parent);
}

// tree traversal
bool rbt_empty(struct rbtree *t){
	return t->root == NULL;
}

struct rbnode *rbt_min(struct rbtree *t){
#ifdef RBTREE_CACHED_MIN
	return t->cached_min;
#else
	struct rbnode *cur = t->root;
	if(cur != NULL){
		while(cur->left != NULL)
			cur = cur->left;
	}
	return cur;
#endif
}

struct rbnode *rbt_max(struct rbtree *t){
	struct rbnode *cur = t->root;
	if(cur != NULL){
		while(cur->right != NULL)
			cur = cur->right;
	}
	return cur;
}

struct rbnode *rbnode_next(struct rbnode *cur){
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

		// one more step to reach the next node
		cur = cur->parent;
	}
	return cur;
}

struct rbnode *rbnode_prev(struct rbnode *cur){
	// same as next_node but in reverse
	if(cur == NULL)
		return NULL;
	if(cur->left != NULL){
		cur = cur->left;
		while(cur->right != NULL)
			cur = cur->right;
	}else{
		while(cur->parent && cur->parent->right != cur)
			cur = cur->parent;
		cur = cur->parent;
	}
	return cur;
}
