#include "../def.h"
#include "../log.h"
#include "../rbtree.h"
#include <stdio.h>

struct test_node{
	struct rbnode n;
	int key;
};

int test_node_cmp(struct rbnode *lhs, struct rbnode *rhs){
	struct test_node *l = (struct test_node*)lhs;
	struct test_node *r = (struct test_node*)rhs;
	return l->key - r->key;
}

#define MAX_NODES 1000000
static struct rbtree t;
static struct test_node nodes[MAX_NODES];

int check_subtree_black_height(struct rbnode *n, int black_height){
	if(n == NULL || n->color == 1)
		black_height += 1;

	if(n != NULL){
		int left_height = check_subtree_black_height(n->left, black_height);
		int right_height = check_subtree_black_height(n->right, black_height);
		if(left_height != right_height)
			LOG_ERROR("left_height = %d, right_height = %d",
				left_height, right_height);
		return left_height;
	}else{
		return black_height;
	}
}

int check_subtree_red_red_count(struct rbnode *n,
		int rr_count, int parent_color){
	if(n != NULL && n->color == 0 && parent_color == 0)
		rr_count += 1;

	if(n != NULL){
		int left_count = check_subtree_red_red_count(n->left, rr_count, n->color);
		int right_count = check_subtree_red_red_count(n->right, rr_count, n->color);
		return left_count + right_count;
	}else{
		return rr_count;
	}
}

int main(int argc, char **argv){
	struct test_node *n;
	int i;

	rbt_init(&t, test_node_cmp);
	for(i = 0; i < MAX_NODES; i += 1){
		n = &nodes[i];
		n->key = i;
		ASSERT(rbt_insert(&t, (struct rbnode*)n));
	}
	LOG("CHECK1:");
	LOG("BLACK HEIGHT: %d", check_subtree_black_height(t.root, 0));
	LOG("RED-RED check: %d", check_subtree_red_red_count(t.root, 0, 1));
	LOG("min: %d", ((struct test_node*)rbt_min(&t))->key);

	for(i = 0; i < MAX_NODES/4; i += 1){
		n = &nodes[i];
		rbt_remove(&t, (struct rbnode*)n);
		LOG("min: %d", ((struct test_node*)rbt_min(&t))->key);
		if(((struct test_node*)rbt_min(&t))->key != (i + 1))
			LOG("min error");
	}
	LOG("CHECK2:");
	LOG("BLACK HEIGHT: %d", check_subtree_black_height(t.root, 0));
	LOG("RED-RED COUNT: %d", check_subtree_red_red_count(t.root, 0, 1));

	getchar();
	return 0;
}
