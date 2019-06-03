#include "outputmessage.h"

#include "../bitset.h"
#include "../system.h"
#include <mutex>
#include <map>
#include <vector>

// NOTE: this may seem inefficient but when we
// increase MESSAGE_ARRAY_LEN, it would scale well
// (testing and measuring required)

#define MESSAGE_ARRAY_LEN 1024
class MessageArray{
private:
	size_t stride;
	void *mem;
	BitSet<MESSAGE_ARRAY_LEN> busy;

public:
	MessageArray(void) : mem(nullptr) {}
	MessageArray(size_t capacity){ init(capacity); }
	MessageArray(MessageArray &&other){
		stride = other.stride;
		mem = other.mem;
		busy = other.busy;

		// empty `other`
		other.mem = nullptr;
	}
	~MessageArray(void){
		cleanup();
	}

	void init(size_t capacity){
		size_t total;
		stride = Message::total_size(capacity);
		total = stride * MESSAGE_ARRAY_LEN;
		mem = sys_aligned_alloc(
			alignof(Message), total);
		if(mem == nullptr)
			UNREACHABLE();
		busy.clear_all();
	}

	void cleanup(void){
		if(mem != nullptr){
			sys_aligned_free(mem);
			mem = nullptr;
		}
	}

	int acquire(void){
		int slot = busy.first_clear();
		if(slot != -1)
			busy.set(slot);
		return slot;
	}

	void release(int slot){
		busy.clear(slot);
	}

	Message *take(int slot){
		DEBUG_CHECK(mem != nullptr,
			"MessageArray::take -> empty array");
		void *ptr = (void*)((char*)(mem) + slot*stride);
		return Message::takeon(ptr, stride);
	}
};

static std::mutex mtx;
static std::vector<MessageArray> array_pool;
static std::multimap<MessageCapacity, uint32> array_indices;

static std::vector<>

static MessageArray &add_array(MessageCapacity capacity, uint32 *arr_idx){
	uint32 next_id = array_pool.size();
	array_indices.insert({capacity, next_id});
	array_pool.emplace_back(capacity);
	if(arr_idx != nullptr) *arr_idx = next_id;
	return array_pool.back();
}

void acquire_output_message(MessageCapacity capacity, OutputMessage *omsg){
	MessageArray *arr;
	uint32 idx, slot;

	std::lock_guard<std::mutex> guard(mtx);
	// try to find a free message with at least
	// the requested capacity
	auto start = array_indices.lower_bound(capacity);
	auto end = array_indices.end();
	for(auto it = start; it != end; ++it){
		idx = it->second;
		arr = &array_pool[idx];
		slot = arr->acquire();
		if(slot != -1){
			omsg->arr_idx = idx;
			omsg->arr_slot = slot;
			omsg->msg = arr->take(slot);
			return;
		}
	}

	// there were no free messages: add new message
	// array to the pool
	arr = &add_array(capacity, &idx);
	omsg->arr_idx = idx;
	omsg->arr_slot = arr->acquire();
	omsg->msg = arr->take(0);
}

void release_output_message(OutputMessage *msg){
	MessageArray *arr;
	std::lock_guard<std::mutex> guard(mtx);
	arr = &array_pool[msg->arr_idx];
	arr->release(msg->arr_slot);
}

