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

// mem.c - thread safe general allocator
bool mem_init(void);
void mem_shutdown(void);
void *mem_alloc(size_t size);
void mem_free(size_t size, void *mem);

#endif //MEM_MEM_H_
