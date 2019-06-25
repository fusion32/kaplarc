#include "outputmessage.h"

#include "../bitset.h"
#include "../system.h"
#include <mutex>
#include <map>
#include <vector>

#define MESSAGE_ARRAY_LEN 768
class MessageArray{
private:
	size_t stride;
	void *mem;
	BitSet<MESSAGE_ARRAY_LEN> busy;

public:
	MessageArray(void) : mem(nullptr) {}
	MessageArray(size_t capacity){ init(capacity); }
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

	// adding move operations will optimize
	// std::vector's performance

	// move assignment
	MessageArray &operator=(MessageArray &&other){
		stride = other.stride;
		mem = other.mem;
		busy = other.busy;
		// empty `other`
		other.mem = nullptr;
		return *this;
	}

	// move construction
	MessageArray(MessageArray &&other){
		//*this = std::move(other);
		operator=(std::move(other));
	}
};

struct Pool{
	MessageCapacity capacity;
	std::vector<MessageArray> arrays;

	MessageArray &grow(uint32 *array_idx){
		if(array_idx != nullptr)
			*array_idx = arrays.size();
		arrays.emplace_back(capacity);
		return arrays.back();
	}

	bool acquire(OutputMessage *omsg){
		uint32 slot;
		uint32 idx = 0;
		for(MessageArray &arr: arrays){
			slot = arr.acquire();
			if(slot != -1){
				omsg->array_idx = idx;
				omsg->array_slot = slot;
				omsg->msg = arr.take(slot);
				return true;
			}
			idx += 1;
		}
		return false;
	}

	// comparing operations will enable sorting
	bool operator<(const Pool &other) const{
		return capacity < other.capacity;
	}
};

static std::mutex mtx;
static std::vector<Pool> pools;
static std::map<MessageCapacity, uint32> indices;

void acquire_output_message(MessageCapacity capacity, OutputMessage *omsg){
	Pool *pool;
	MessageArray *arr;
	uint32 pool_idx;
	uint32 array_idx;

	std::lock_guard<std::mutex> guard(mtx);

	// try to find a pool with the
	// requested capacity or create
	// a new one
	auto it = indices.find(capacity);
	if(it == indices.end()){
		pool_idx = pools.size();
		pools.emplace_back();
		indices.insert({capacity, pool_idx});
	}else{
		pool_idx = it->second;
	}

	// try to acquire a message from the
	// arrays in the selected pool
	pool = &pools[pool_idx];
	if(pool->acquire(omsg)){
		omsg->pool_idx = pool_idx;
		return;
	}

	// if there were no available messages,
	// increase the pool and take the first
	// message
	arr = &pool->grow(&array_idx);
	omsg->pool_idx = pool_idx;
	omsg->array_idx = array_idx;
	omsg->array_slot = arr->acquire();
	omsg->msg = arr->take(0);
}

void release_output_message(OutputMessage *omsg){
	Pool *pool;
	MessageArray *arr;
	std::lock_guard<std::mutex> guard(mtx);
	pool = &pools[omsg->pool_idx];
	arr = &pool->arrays[omsg->array_idx];
	arr->release(omsg->array_slot);
}

