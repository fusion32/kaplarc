#include "mem.h"
#include "../system.h"

//@TODO: create a test

struct mem_arena{
	uint8 *ptr;
	uint8 *end;
	uint8 mem[];
};

struct mem_arena *mem_arena_create(size_t capacity){
	struct mem_arena *a = sys_malloc(sizeof(struct mem_arena) + capacity);
	a->ptr = a->mem;
	a->end = a->mem + capacity;
	return a;
}

void mem_arena_destroy(struct mem_arena *a){
	sys_free(a);
}

void *mem_arena_aligned_alloc(struct mem_arena *a,
		size_t size, size_t alignment){
	DEBUG_ASSERT(IS_POWER_OF_TWO(alignment));
	size_t alignment_mask = alignment - 1;
	uintptr ptr = (uintptr)a->ptr;
	uintptr nextptr;

	// forward align ptr
	if(ptr & alignment_mask)
		ptr = (ptr + alignment_mask) & ~alignment_mask;
	// check if there is memory left
	nextptr = ptr + size;
	if(nextptr >= ((uintptr)a->end))
		return NULL;
	// update arena pointer and return
	a->ptr = (void*)nextptr;
	return (void*)ptr;
}

#define PTR_ALIGNMENT (sizeof(void*))
INLINE void *mem_arena_alloc(struct mem_arena *a, size_t size){
	return mem_arena_aligned_alloc(a, size, PTR_ALIGNMENT);
}

void mem_arena_reset(struct mem_arena *a){
	a->ptr = a->mem;
}
