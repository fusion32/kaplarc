#ifndef TREE_HPP_
#define TREE_HPP_

#include "tree.h"

template<typename T>
class TreeIterator{
private:
	struct tree_node *cur;

public:
	TreeIterator(void) : cur(nullptr) {}
	TreeIterator(struct tree_node *n) : cur(n) {}

	struct tree_node *node(void) const{
		return cur;
	}

	T &operator*(void) const{
		return *(T*)tree_node_key(cur);
	}

	T *operator->(void) const{
		return (T*)tree_node_key(cur);
	}

	bool operator==(const TreeIterator &rhs) const{
		return cur == rhs.cur;
	}

	bool operator!=(const TreeIterator &rhs) const{
		return cur != rhs.cur;
	}

	TreeIterator operator+(int steps) const{
		struct tree_node *next = cur;
		while(steps > 0 && next != NULL){
			next = tree_node_next(next);
			steps -= 1;
		}
		return next;
	}

	TreeIterator operator-(int steps) const{
		struct tree_node *prev = cur;
		while(steps > 0 && next != NULL){
			prev = tree_node_prev(prev);
			steps -= 1;
		}
		return prev;
	}

	TreeIterator operator++(int){
		struct tree_node *prev = cur;
		cur = tree_node_next(cur);
		return prev;
	}

	TreeIterator &operator++(void){
		cur = tree_node_next(cur);
		return *this;
	}

	TreeIterator operator--(int){
		struct tree_node *next = cur;
		cur = tree_node_prev(cur);
		return next;
	}

	TreeIterator &operator--(void){
		cur = tree_node_prev(cur);
		return *this;
	}
};

template<typename T, typename Compare = std::less<T>>
class Tree{
protected:
	// internal tree structure
	struct tree *self;

	// tree comparison function
	static int T_cmp(void *a, void *b){
		static Compare cmp;
		const T &lhs = *(T*)a;
		const T &rhs = *(T*)b;
		if(cmp(lhs, rhs))
			return -1;
		else if(cmp(rhs, lhs))
			return 1;
		else
			return 0;
	}

	template<typename... Args>
	void construct_key(void *key, Args&&... args){
		new(key) T(std::forward<Args>(args)...);
	}

	template<typename... Args>
	void construct_key(const TreeIterator<T> &it, Args&&... args){
		new(tree_node_key(it.node())) T(std::forward<Args>(args)...);
	}

	void destruct_key(void *key){
		((T*)key)->~T();
	}

	void destruct_key(const TreeIterator<T> &it){
		((T*)tree_node_key(it.node()))->~T();
	}

public:
	Tree(void) : self(nullptr) {};
	~Tree(void){ destroy(); }

	bool ready(void) const{
		return self != nullptr;
	}

	operator bool(void) const{
		return ready();
	}

	bool create(long slots){
		if(self != nullptr)
			return false;

		// create tree structure
		self = tree_create(slots, sizeof(T), T_cmp);
		return self != nullptr;
	}

	void destroy(void){
		if(self != nullptr){
			// iterate over the tree and call the
			// destructor on every element
			for(auto it = begin(); it != end(); ++it)
				destruct_key(it);

			// release tree structure
			tree_destroy(self);
			self = nullptr;
		}
	}

	bool empty(void){
		return tree_empty(self) != 0;
	}

	TreeIterator<T> min(void){
		return tree_min(self);
	}

	TreeIterator<T> max(void){
		return tree_max(self);
	}

	template<typename G>
	TreeIterator<T> find(const G &key){
		return tree_find_node(self, (void*)&key);
	}

	template<typename... Args>
	TreeIterator<T> insert(Args&&... args){
		// allocate node
		struct tree_node *node = tree_alloc_node(self);
		if(node == nullptr)
			return nullptr;

		// initialize node
		node->parent = nullptr;
		node->left = nullptr;
		node->right = nullptr;
		node->data = 0;
		construct_key(node->key, std::forward<Args>(args));

		// insert node
		tree_insert_node(self, node);
		return node;
	}

	TreeIterator<T> erase(const TreeIterator<T> &it){
		auto next = it + 1;
		remove(it);
		return next;
	}

	void remove(const TreeIterator<T> &it){
		destruct_key(it);
		tree_remove(self, it.node());
	}

	TreeIterator<T> begin(void){
		return min();
	}

	TreeIterator<T> end(void){
		return nullptr;
	}
};

#endif //TREE_HPP_
