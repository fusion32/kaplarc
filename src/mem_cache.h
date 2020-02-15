#ifndef MEM_CACHE_H_
#define MEM_CACHE_H_

#include "def.h"

struct mem_cache;
struct mem_cache *mem_cache_create(uint slab_slots, uint slab_stride);
void mem_cache_destroy(struct mem_cache *sc);
void *mem_cache_alloc(struct mem_cache *sc);
void mem_cache_free(struct mem_cache *sc, void *ptr);
int mem_cache_shrink(struct mem_cache *sc);

#endif //MEM_CACHE_H_
