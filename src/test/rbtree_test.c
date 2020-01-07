#ifdef BUILD_TEST

#include "../def.h"
#include "../log.h"
#include "../rbtree.h"

struct test_node{
	struct rbnode rbn;
	int key;
};

int test_node_cmp(struct rbnode *lhs, struct rbnode *rhs){
	struct test_node *l = (struct test_node*)lhs;
	struct test_node *r = (struct test_node*)rhs;
	return l->key - r->key;
}

// just to avoid compiler warnings
#define RBTEST_MIN(t)		RBT_MIN(struct test_node, (t))
#define RBTEST_FIND(t, n)	RBT_FIND(struct test_node, (t), (n))
#define RBTEST_INSERT		RBT_INSERT
#define RBTEST_REMOVE		RBT_REMOVE

#define MAX_NODES 10000
static struct rbtree tree;
static struct test_node nodes[MAX_NODES];

static int check_black_height(struct rbnode *n, int cur){
	int left, right;
	if(n == NULL || n->color == 1)
		cur += 1;

	if(n != NULL){
		left = check_black_height(n->left, cur);
		if(left < 0) return -1; // propagate error

		right = check_black_height(n->right, cur);
		if(right < 0) return -1; // propagate error

		if(left != right){
			LOG_ERROR("left = %d, right = %d",
				left, right);
			return -1;
		}
		return left;
	}else{
		return cur;
	}
}

static int check_red_red(struct rbnode *n, int parent_color){
	if(n != NULL){
		if(n->color == 0 && parent_color == 0)
			return -1;
		// propagate error
		if(check_red_red(n->left, n->color) < 0)
			return -1;
		// propagate error
		if(check_red_red(n->right, n->color) < 0)
			return -1;
	}
	return 0;
}

static bool rbtree_check(struct rbtree *t, int expected_min){
	if(check_black_height(t->root, 0) < 0){
		LOG_ERROR("rbtree_check: black height varies");
		return false;
	}

	if(check_red_red(t->root, 0) < 0){
		LOG_ERROR("rbtree_check: red red pair detected");
		return false;
	}

	int min = RBTEST_MIN(t)->key;
	if(min != expected_min){
		LOG_ERROR("rbtree_check: invalid minimum value"
			" (min = %d, expected = %d)", min, expected_min);
		return false;
	}
	return true;
}

#define KEY_OFFSET (-MAX_NODES/2)
bool rbtree_test(void){
	struct test_node *n;
	int i;

	rbt_init(&tree, test_node_cmp);
	for(i = 0; i < MAX_NODES; i += 1){
		n = &nodes[i];
		n->key = i + KEY_OFFSET;
		if(!RBTEST_INSERT(&tree, n)){
			LOG_ERROR("rbtree_test: failed to insert node");
			return false;
		}
	}
	if(!rbtree_check(&tree, KEY_OFFSET)){
		LOG_ERROR("rbtree_test: check failed after insertion");
		return false;
	}

	for(i = 0; i < MAX_NODES/4; i += 1){
		n = &nodes[i];
		RBTEST_REMOVE(&tree, n);

		// check tree properties as we remove elements
		if(!rbtree_check(&tree, (i + 1 + KEY_OFFSET))){
			LOG_ERROR("rbtree_test: check failed while"
				"removing elements (i = %d)", i);
			return false;
		}
	}
	return true;
}

#endif //BUILD_TEST
