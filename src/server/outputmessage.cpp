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
		stride = message_total_size(capacity);
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
		return message_takeon(ptr, stride);
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

	bool acquire(OutputMessage *out){
		uint32 slot;
		uint32 idx = 0;
		for(MessageArray &arr: arrays){
			slot = arr.acquire();
			if(slot != -1){
				out->array_idx = idx;
				out->array_slot = slot;
				out->msg = arr.take(slot);
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

void output_message_acquire(MessageCapacity capacity, OutputMessage *out){
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
	if(pool->acquire(out)){
		out->pool_idx = pool_idx;
		return;
	}

	// if there were no available messages,
	// increase the pool and take the first
	// message
	arr = &pool->grow(&array_idx);
	out->pool_idx = pool_idx;
	out->array_idx = array_idx;
	out->array_slot = arr->acquire();
	out->msg = arr->take(0);
}

void output_message_release(OutputMessage *out){
	Pool *pool;
	MessageArray *arr;
	std::lock_guard<std::mutex> guard(mtx);
	pool = &pools[out->pool_idx];
	arr = &pool->arrays[out->array_idx];
	arr->release(out->array_slot);
}

