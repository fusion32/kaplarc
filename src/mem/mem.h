#ifndef MEM_H_
#define MEM_H_

#include "../def.h"

// arena.c - arena allocator
struct arena;
struct arena *mem_arena_create(size_t capacity);
void mem_arena_destroy(struct arena *a);
void *mem_arena_aligned_alloc(struct arena *a,
		size_t size, size_t alignment);
void *mem_arena_alloc(struct arena *a, size_t size);
void mem_arena_reset(struct arena *s);

// mem.c - thread safe general allocator
bool mem_init(void);
void mem_shutdown(void);
void *mem_alloc(size_t size);
void mem_free(size_t size, void *mem);

#endif //MEM_H_
