#ifndef MEM_MEM_H_
#define MEM_MEM_H_

#include "../def.h"

// arena.c - arena allocator
struct mem_arena;
struct mem_arena *mem_arena_create(size_t capacity);
void mem_arena_destroy(struct mem_arena *a);
void *mem_arena_aligned_alloc(struct mem_arena *a,
		size_t size, size_t alignment);
void *mem_arena_alloc(struct mem_arena *a, size_t size);
void mem_arena_reset(struct mem_arena *a);

// array.c - dynamic array
struct mem_array{
	void *data;
	uint32 stride;
	uint32 count;
	uint32 capacity;
};
#define MEM_ARRAY_INITIALIZER(stride) ((struct mem_array){NULL, (stride), 0, 0})
#define MEM_ARRAY_INIT(s) {NULL, (s), 0, 0}
void mem_array_init(struct mem_array *arr, uint32 stride);
void mem_array_reserve(struct mem_array *arr, uint32 capacity);
void *mem_array_push_back(struct mem_array *arr);
void mem_array_pop_back(struct mem_array *arr);
void *mem_array_at(struct mem_array *arr, uint32 idx);
void *mem_array_front(struct mem_array *arr);
void *mem_array_back(struct mem_array *arr);
uint32 mem_array_reset(struct mem_array *arr);

// mem.c - thread safe general allocator
bool mem_init(void);
void mem_shutdown(void);
void *mem_alloc(size_t size);
void mem_free(size_t size, void *mem);

#endif //MEM_MEM_H_
