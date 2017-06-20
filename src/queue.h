#ifndef LIST_H_
#define LIST_H_

#include "mmblock.h"

namespace kp{

//NOTE: - the type T must implement a `T *next` member
//	- the elements of the queue are increasingly
//	  ordered
template<typename T, int N>
class queue{
private:
	// list control
	T *head;
	int length;

	// list memory
	mmblock<T, N> blk;

public:
	struct iterator{
		// iterator element
		T *elm;

		// iterator operations
		bool operator==(iterator &rhs){ return (lhs.elm == rhs.elm); }
		bool operator!=(iterator &rhs){ return (elm != rhs.elm); }
		iterator &operator=(iterator &rhs){
			elm = rhs.elm;
			return *this;
		}
		iterator &operator++(void){
			if(elm != nullptr)
				elm = elm->next;
			return *this;
		}
		iterator operator++(int){
			iterator prev = *this;
			if(elm != nullptr)
				elm = elm->next;
			return prev;
		}
		T &operator*(void){
			return *elm;
		}
		T *operator->(void){
			return elm;
		}
	};

	// iterator end points
	iterator begin(void){ return iterator{head}; }
	iterator end(void){ return iterator{nullptr}; }


	// contructor
	queue(void) : head(nullptr), length(0), blk() {}
	~queue(void){}

	T &front(void){
		return *head;
	}

	void pop_front(void){
		if(!head) return;
		T *it = head;
		head = head->next;
		blk.free(it);
	}

	template<typename G>
	iterator insert(G &&value){
		T *entry, **it;
		// allocate new entry
		entry = blk.alloc();
		if(entry == nullptr){
			LOG_ERROR("queue::insert: reached maximum capacity (%d)", N);
			return end();
		}
		// insert new element
		it = &head;
		*entry = std::forward<G>(value);
		while(*it != nullptr && **it < *entry)
			it = &(*it)->next;
		entry->next = *it;
		*it = entry;
		return iterator{entry};
	}

	iterator erase(iterator &pos){
		T **it;
		// check if iterator is still valid
		it = &head;
		while(*it != nullptr && *it != pos.elm)
			it = &(*it)->next;
		if(*it == nullptr){
			LOG_ERROR("queue::erase: trying to remove invalid entry");
			return end();
		}
		// remove entry
		*it = *it->next;
		blk.free(pos.node);
		return iterator{*it};
	}

};

} //namespace

#endif //LIST_H_
