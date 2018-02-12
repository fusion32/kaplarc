#ifndef IOCP_IMPL_OPERATION_H_
#define IOCP_IMPL_OPERATION_H_

#include <atomic>
#include <functional>
#include <system_error>
#include "winsock.h"

namespace net{
class socket;

enum{
	OP_NONE = 0,
	OP_ACCEPT,
	OP_READ,
	OP_WRITE,
};
using operation_callback =
	std::function<void(const std::error_code&, std::size_t)>;
struct operation : public OVERLAPPED{
	socket			*socket;
	std::atomic<int>	opcode;
	operation_callback	complete;
	operation		*next;
};

class op_queue{
private:
	operation *head;
	operation *tail;

public:
	op_queue(void) : head(nullptr), tail(nullptr) {}
	~op_queue(void) {}

	operation *front(void) { return head; }
	operation *back(void) { return tail; }

	void push(operation *op){
		op->next = nullptr;
		if(tail == nullptr){
			head = op;
			tail = op;
		}else{
			tail->next = op;
			tail = op;
		}

	}

	void pop(void){
		if(head != nullptr){
			head = head->next;
			if(head == nullptr)
				tail = nullptr;
		}
	}
};

}; //namespace net

#endif //IOCP_IMPL_ASYNC_OP_H_
