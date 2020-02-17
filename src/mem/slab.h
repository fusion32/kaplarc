#ifndef MEM_INTERNAL_H_
#define MEM_INTERNAL_H_ 1

#include "../def.h"
#include "../system.h"

// slab.c
struct slab{
	uint stride;
	uint capacity;
	uint offset;
	uint freecount;
	void *freelist;
	void *mem;
	struct slab *next;
};
struct slab *slab_create(uint slots, uint stride);
void slab_destroy(struct slab *s);
void *slab_alloc(struct slab *s);
bool slab_free(struct slab *s, void *ptr);
bool slab_is_full(struct slab *s);
bool slab_is_empty(struct slab *s);

// slab_cache.c
struct slab_cache;
struct slab_cache *slab_cache_create(uint slab_slots, uint slab_stride);
void slab_cache_destroy(struct slab_cache *c);
void *slab_cache_alloc(struct slab_cache *c);
void slab_cache_free(struct slab_cache *c, void *ptr);
int slab_cache_shrink(struct slab_cache *c);

#endif //MEM_INTERNAL_H_
