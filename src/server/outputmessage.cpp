#include "outputmessage.h"
#include "message.h"
#include <string.h>
#include <map>
#include <vector>
#include <mutex>

class MessageStack{
private:
	const size_t MSG_CAPACITY;
	std::vector<Message*> messages;

public:
	MessageStack(size_t message_capacity,
		size_t initial_capacity = 64)
	 : MSG_CAPACITY(message_capacity) {
		Message *msg;
		messages.reserve(initial_capacity);
		for(int i = 0; i < initial_capacity; ++i){
			msg = new Message(MSG_CAPACITY);
			messages.push_back(msg);
		}
	}

	~MessageStack(void){
		for(Message *msg : messages)
			delete msg;
		messages.clear();
	}

	Message *acquire(void){
		Message *msg;
		if(messages.empty()){
			msg = new Message(MSG_CAPACITY);
		}else{
			msg = messages.back();
			messages.pop_back();
		}
		return msg;
	}

	void release(Message *msg){
		// return message to the pool
		messages.push_back(msg);
	}
};

#define ROUND_TO_64(x) (((x) + 63) & ~63)
static std::mutex mtx;
static std::map<size_t, MessageStack> pool;
static Message *pool_acquire(size_t capacity){
	capacity = ROUND_TO_64(capacity);
	std::lock_guard<std::mutex> lock(mtx);
	auto it = pool.find(capacity);
	if(it == pool.end()){
		// create new stack
		auto ret = pool.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(capacity),
			std::forward_as_tuple(capacity));
		if(ret.second == false)
			return nullptr;
		it = ret.first;
	}
	return it->second.acquire();
}
static void pool_release(Message *msg){
	std::lock_guard<std::mutex> lock(mtx);
	auto it = pool.find(msg->capacity);
	if(it == pool.end()){
		UNREACHABLE();
		return;
	}
	it->second.release(msg);
}

OutputMessage output_message(size_t capacity){
	return OutputMessage(
		pool_acquire(capacity),
		pool_release);
}

