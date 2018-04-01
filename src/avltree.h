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

// avl tree utility
void avl_insert_node(struct tree *t, struct tree_node *n);
void avl_remove_node(struct tree *t, struct tree_node *n);
void avl_retrace(struct tree *t, struct tree_node *n);

// avl tree node specific utility
struct tree_node *avl_node_rotate_left(struct tree_node *a);
struct tree_node *avl_node_rotate_right(struct tree_node *a);

#if 0
template<typename T, long N>
class AVLTree{
private:
	struct node{
		T key;
		int height;
		node *parent;
		node *left;
		node *right;
	};

	// tree root
	node *root;

	// tree memory
	node *node_pool[N];
	struct memblock *blk;

	// fix node height
	static void fix_height(node *x){
		int height = 0;
		if(x->right != nullptr)
			height = x->right->height;
		if(x->left != nullptr && x->left->height > height)
			height = x->left->height;
		x->height = height + 1;
	}

	// calc node balance factor
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

	// retrace from node `x` and returns the new root node
	static node *retrace(node *x){
		node *parent;
		int b1;
		for(;;){
			parent = x->parent;
			fix_height(x);
			b1 = balance_factor(x);
			if(b1 >= 2){
				if(balance_factor(x->right) <= -1)
					/*x->right = */rotate_right(x->right, x->right->left);
				x = rotate_left(x, x->right);
			}else if(b1 <= -2){
				if(balance_factor(x->left) >= 1)
					/*x->left = */rotate_left(x->left, x->left->right);
				x = rotate_right(x, x->left);
			}

			if(parent == nullptr)
				break;
			x = parent;
		}
		return x;
	}

	// insert node into the tree
	void insert_node(node *x){
		if(root != nullptr){
			node **y = &root;
			while(*y != nullptr){
				x->parent = (*y);
				if(x->key < (*y)->key)
					y = &(*y)->left;
				else
					y = &(*y)->right;
			}
			*y = x;
			root = retrace(x);
		} else {
			root = x;
		}
	}

	// remove node from the tree
	void remove_node(node *x){
		node *y = nullptr;
		node *retrace_from = nullptr;
		if(x->left != nullptr && x->right != nullptr){
			if(x->height == 2){
				y = x->right;
				y->left = x->left;
				y->left->parent = y;
				retrace_from = y;
			}else{
				// find smallest element y on the x->right subtree
				y = x->right;
				while(y->left != nullptr)
					y = y->left;

				// pop y from the tree
				if(y->right != nullptr)
					y->right->parent = y->parent;
				if(y->parent->left == y)
					y->parent->left = y->right;
				else
					y->parent->right = y->right;

				// replace x with y
				y->left = x->left;
				y->left->parent = y;
				y->right = x->right;
				// x->right may have become null from the y pop
				if(y->right) y->right->parent = y;
				retrace_from = y->parent;
			}
		}else if(x->left != nullptr){
			y = x->left;
			retrace_from = y;
		}else if(x->right != nullptr){
			y = x->right;
			retrace_from = y;
		}

		// fix parent link
		if(y != nullptr)
			y->parent = x->parent;
		else
			retrace_from = x->parent;

		if(x->parent){
			if(x->parent->left == x)
				x->parent->left = y;
			else
				x->parent->right = y;
		}

		// perform retrace
		if(retrace_from != nullptr)
			root = retrace(retrace_from);
		else
			root = nullptr;
	}

	// `G` is anything that can be compared to `T`
	template<typename G>
	node *find_node(const G &value){
		node *x = root;
		while(x != nullptr && !(x->key == value)){
			if(x->key < value)
				x = x->right;
			else
				x = x->left;
		}
		return x;
	}

public:
	class iterator{
	private:
		AVLTree	*tree;
		node	*cur;
		friend AVLTree;

	public:
		iterator(void) : tree(nullptr), cur(nullptr) {}
		iterator(AVLTree *tree_) : tree(tree_), cur(nullptr) {}
		iterator(AVLTree *tree_, node *cur_) : tree(tree_), cur(cur_) {}
		iterator(const iterator &it) : tree(it.tree), cur(it.cur) {}

		T &operator*(void) const{
			return cur->key;
		}

		T *operator->(void) const{
			return &cur->key;
		}

		bool operator==(const iterator &rhs) const{
			return tree == rhs.tree && cur == rhs.cur;
		}

		bool operator!=(const iterator &rhs) const{
			return tree != rhs.tree || cur != rhs.cur;
		}

		iterator &operator++(void){
			if(tree == nullptr)
				return *this;

			// wrap around behaviour
			if(cur == nullptr){
				cur = tree->root;
				if(cur != nullptr){
					while(cur->left != nullptr)
						cur = cur->left;
				}
				return *this;
			}

			// find next element
			if(cur->right != nullptr){
				// elements from the right subtree are all
				// greater than the current node so we need
				// to find the leftmost(smallest) element
				// from that subtree
				cur = cur->right;
				while(cur->left != nullptr)
					cur = cur->left;
			}else{
				// if there is no right subtree, we need to
				// iterate back to a parent greater than its
				// previous element
				while(cur->parent && cur->parent->left != cur)
					cur = cur->parent;

				if(cur->parent && cur->parent->left == cur)
					cur = cur->parent;
				else
					cur = nullptr;
			}
			return *this;
		}

		iterator &operator--(void){
			if(tree == nullptr)
				return *this;

			// wrap around behaviour
			if(cur == nullptr){
				cur = tree->root;
				if(cur != nullptr){
					while(cur->right != nullptr)
						cur = cur->right;
				}
				return *this;
			}

			// find previous element
			if(cur->left != nullptr){
				cur = cur->left;
				while(cur->right != nullptr)
					cur = cur->right;
			}else{
				while(cur->parent && cur->parent->right != cur)
					cur = cur->parent;
				if(cur->parent && cur->parent->right == cur)
					cur = cur->parent;
				else
					cur = nullptr;
			}
			return *this;
		}
	};

	// delete copy and move operations
	AVLTree(const AVLTree&)			= delete;
	AVLTree(AVLTree&&)			= delete;
	AVLTree &operator=(const AVLTree&)	= delete;
	AVLTree &operator=(AVLTree&&)		= delete;

	AVLTree(void) : root(nullptr){
		blk = memblock_create1(node_pool,
			sizeof(node) * N, sizeof(node));
	}
	~AVLTree(void){
		memblock_destroy(blk);
	}

	template<typename G>
	iterator insert(G &&value){
		// allocate node
		node *x = (node*)memblock_alloc(blk);
		if(x == nullptr)
			return iterator(this);

		// initialize node
		x->key = std::forward<G>(value);
		x->height = 1;
		x->parent = nullptr;
		x->left = nullptr;
		x->right = nullptr;

		// insert into the tree
		insert_node(x);
		return iterator(this, x);
	}

	template <typename... Args>
	iterator emplace(Args&&... args){
		// allocate node
		node *x = (node*)memblock_alloc(blk);
		if(x == nullptr)
			return iterator(this);

		// initialize node
		new(&x->key) T{std::forward<Args>(args)...};
		x->height = 1;
		x->parent = nullptr;
		x->left = nullptr;
		x->right = nullptr;

		// insert into the tree
		insert_node(x);
		return iterator(this, x);
	}

	void remove(iterator it){
		node *x = it.cur;
		if(x == nullptr)
			return;
		remove_node(x);
		x->key.~T();
		memblock_free(blk, x);
	}

	iterator erase(iterator it){
		node *x = it.cur;
		iterator next = ++it;
		if(x == nullptr)
			return iterator(this);
		remove_node(x);
		x->key.~T();
		memblock_free(blk, x);
		return next;
	}

	bool empty(void){
		return (root == nullptr);
	}

	int height(void){
		if(root == nullptr) return 0;
		return root->height;
	}

	template<typename G>
	bool contains(const G &value){
		return (find_node(value) != nullptr);
	}

	template<typename G>
	iterator find(const G &value){
		node *x = find_node(value);
		return iterator(this, x);
	}

	iterator begin(void){
		if(root == nullptr)
			return iterator(this);

		node *x = root;
		while(x->left != nullptr)
			x = x->left;
		return iterator(this, x);
	}

	iterator rbegin(void){
		if(root == nullptr)
			return iterator(this);

		node *x = root;
		while(x->right != nullptr)
			x = x->right;
		return iterator(this, x);
	}

	iterator end(void){
		return iterator(this);
	}
};
#endif // 0

#endif //AVLTREE_H_
