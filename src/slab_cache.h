#ifndef SLAB_CACHE_H_
#define SLAB_CACHE_H_

#include "def.h"

struct slab_cache;
struct slab_cache *slab_cache_create(uint32 slab_slots, uint32 slab_stride);
void slab_cache_destroy(struct slab_cache *sc);
void *slab_cache_alloc(struct slab_cache *sc);
bool slab_cache_free(struct slab_cache *sc, void *ptr);
int slab_cache_shrink(struct slab_cache *sc);

#endif //SLAB_CACHE_H_
