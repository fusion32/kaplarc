#include "mem.h"
#include "../system.h"

#define PTR_ALIGNMENT_MASK (sizeof(void*) - 1)
void mem_array_init(struct mem_array *arr, uint32 stride){
	DEBUG_ASSERT(!(stride & PTR_ALIGNMENT_MASK));
	arr->data = NULL;
	arr->stride = stride;
	arr->count = 0;
	arr->capacity = 0;
}

#define MEM_ARRAY_INITIAL_CAPACITY 32
#define MEM_ARRAY_GROWTH_RATE 1.5f
static void mem_array_grow(struct mem_array *arr){
	uint32 new_capacity;
	size_t new_size;
	if(arr->data == NULL){
		new_capacity = MEM_ARRAY_INITIAL_CAPACITY;
		new_size = new_capacity * arr->stride;
		arr->data = sys_malloc(new_size);
		arr->capacity = new_capacity;
	}else{
		new_capacity = (size_t)(arr->capacity * MEM_ARRAY_GROWTH_RATE);
		new_size = new_capacity * arr->stride;
		arr->data = sys_realloc(arr->data, new_size);
		arr->capacity = new_capacity;
	}
}

void mem_array_reserve(struct mem_array *arr, uint32 capacity){
	size_t new_size;
	if(arr->data == NULL){
		new_size = capacity * arr->stride;
		arr->data = sys_malloc(new_size);
		arr->capacity = capacity;
	}else if(capacity > arr->capacity){
		new_size = capacity * arr->stride;
		arr->data = sys_realloc(arr->data, new_size);
		arr->capacity = capacity;
	}
}

void *mem_array_push_back(struct mem_array *arr){
	uint32 offset;
	if(arr->count >= arr->capacity)
		mem_array_grow(arr);
	offset = arr->count * arr->stride;
	arr->count += 1;
	return OFFSET_POINTER(arr->data, offset);
}

void mem_array_pop_back(struct mem_array *arr){
	if(arr->count > 0)
		arr->count -= 1;
}

void *mem_array_at(struct mem_array *arr, uint32 idx){
	return OFFSET_POINTER(arr->data, idx * arr->stride);
}

void *mem_array_front(struct mem_array *arr){
	DEBUG_ASSERT(arr->count > 0);
	return mem_array_at(arr, 0);
}

void *mem_array_back(struct mem_array *arr){
	DEBUG_ASSERT(arr->count > 0);
	return mem_array_at(arr, arr->count - 1);
}

uint32 mem_array_reset(struct mem_array *arr){
	uint32 count = arr->count;
	arr->count = 0;
	return count;
}

#if 0
void mem_array_swap(struct mem_array *arr, uint32 idx1, uint32 idx2){
	DEBUG_ASSERT(idx1 < arr->count);
	DEBUG_ASSERT(idx2 < arr->count);
	DEBUG_ASSERT(arr->stride <= 256);

	uint8 tmp[256];
	size_t stride = arr->stride;
	void *ptr1 = OFFSET_POINTER(arr->data, idx1 * arr->stride);
	void *ptr2 = OFFSET_POINTER(arr->data, idx2 * arr->stride);
	memcpy(tmp, ptr1, stride);
	memcpy(ptr1, ptr2, stride);
	memcpy(ptr2, tmp, stride);
}
#endif
