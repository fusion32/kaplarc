// C compatible source: don't change if you are
// going to break compatibility

// NOTE: this interface is totally compatible with the `tree` object
// and only extends a few of the functions to properly balance the
// tree in the avl form

#ifndef AVLTREE_H_
#define AVLTREE_H_

struct tree;
struct tree_node;

// avl tree interface
struct tree_node *avl_insert(struct tree *t, void *key);
struct tree_node *avl_erase(struct tree *t, struct tree_node *n);
void avl_remove(struct tree *t, struct tree_node *n);
int avl_height(struct tree *t);

// avl tree utility
void avl_insert_node(struct tree *t, struct tree_node *n);
void avl_remove_node(struct tree *t, struct tree_node *n);
void avl_retrace(struct tree *t, struct tree_node *n);

// avl tree node specific utility
struct tree_node *avl_node_rotate_left(struct tree_node *a);
struct tree_node *avl_node_rotate_right(struct tree_node *a);

#endif //AVLTREE_H_
