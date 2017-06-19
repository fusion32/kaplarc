#ifndef LIST_H_
#define LIST_H_

#include "mmblock.h"

namespace kpl{

//NOTE: this might not be the best approach.
//	try to use a custom allocator with STL
//	list/forward_list
template<typename T, int N>
class list{
private:
	struct node{
		T	item;
		node	*next;
	};

	// list control
	node *head;
	int length;

	// list memory
	mmblock<node, N> blk;

public:

	class iterator{
	private:
		node *elm;

	public:
		// constructors
		iterator(void) : elm(nullptr) {}
		iterator(const iterator &it) : elm(it.elm) {}
		iterator(const node &it) : elm(&it) {}
		iterator(node *it) : elm(it) {}

		// destructor
		~iterator(void){}

		// iterator operations
		inline void advance(void){
			if(elm != nullptr)
				elm = elm->next;
		}

		bool operator==(iterator &rhs){ return (elm == rhs.elm); }
		bool operator!=(iterator &rhs){ return (elm != rhs.elm); }

		iterator &operator=(const iterator &rhs){
			elm = rhs.elm;
			return *this;
		}

		iterator &operator=(const node &rhs){
			elm = &rhs;
			return *this;
		}

		iterator &operator=(const node *rhs){
			elm = rhs;
			return *this;
		}

		iterator &operator++(void){
			advance();
			return *this;
		}

		iterator operator++(int){
			iterator it = *this;
			advance();
			return it;
		}

		T &operator*(void){
			static T placeholder;
			if(elm == nullptr)
				return placeholder;
			return elm->item;
		}

		T *operator->(void){
			static T placeholder;
			if(elm == nullptr)
				return &placeholder;
			return &elm->item;
		}
	};

	// iterator end points
	iterator begin(void){ return iterator(head); }
	iterator end(void){ return iterator(); }

	// delete copy and move operations
	list(const list &l)		= delete;
	list &operator=(const list &l)	= delete;
	list(list &&l)			= delete;
	list &operator=(list &&l)	= delete;

	// constructor/destructor
	list(void) : head(nullptr), length(0), blk() {}
	~list(void){}


	// list operations
	bool empty(void){
		return (length == 0);
	}

	void clear(void){
		blk.reset();
		head = nullptr;
		length = 0;
	}

	T &front(void){
		return *head;
	}

	template<typename G>
	void push_front(G &&value){
		node *elm = blk.alloc();
		if(elm == nullptr){
			LOG_ERROR("push: list is at maximum capacity (%d)", N);
			return;
		}

		elm->item = std::forward<G>(value);
		elm->next = head;
		head = elm;
		length += 1;
	}

	void pop_front(void){
		node *tmp;
		if(head != nullptr){
			tmp = head;
			head = head->next;
			blk.free(tmp);
			length -= 1;
		}
	}

	template<typename G>
	iterator insert_after(const iterator &pos, G &&value){
		node *elm = blk.alloc();
		if(elm == nullptr){
			LOG_ERROR("insert_after: list is at maximum capacity (%d)", N);
			return end();
		}

		elm->item = std::forward<G>(value);
		if(pos == end()){
			// insert at the end
			node **it = &head;
			while(*it != nullptr)
				it = &(*it)->next;

			*it = elm;
			elm->next = nullptr;
		}
		else{
			// insert after pos
			elm->next = pos.elm->next;
			pos.elm->next = elm;
		}

		length += 1;
		return iterator(elm);
	}

	iterator erase_after(iterator pos){
		node *tmp;
		if(pos == end() || pos.elm->next == nullptr)
			return;

		tmp = pos.elm->next;
		pos.elm->next = tmp->next;
		blk.free(tmp);
		length -= 1;
		return ++pos;
	}
};

} //namespace

#endif //LIST_H_
