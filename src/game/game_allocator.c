#include "game_allocator.h"
#include "../slab_cache.h"

static struct slab_cache *caches[NUM_CACHES];
static struct {
	uint32 slots;
	uint32 stride;
} cache_map[NUM_CACHES] = {
	// roughly 8K per slab
	[CACHE_64] = {.slots = 128, .stride = 64},
	[CACHE_128] = {.slots = 64, .stride = 128},
	[CACHE_256] = {.slots = 32, .stride = 256},
	[CACHE_512] = {.slots = 16, .stride = 512},
	[CACHE_1024] = {.slots = 8, .stride = 1024},
};

bool game_allocator_init(void){
	for(int i = CACHE_FIRST; i <= CACHE_LAST; i += 1){
		caches[i] = slab_cache_create(
			cache_map[i].slots, cache_map[i].stride);
		ASSERT(caches[i] != NULL);
	}
	return true;
}

void game_allocator_shutdown(void){
	for(int i = CACHE_FIRST; i <= CACHE_LAST; i += 1)
		slab_cache_destroy(caches[i]);
}

void *game_alloc(cache_idx_t cache){
	return slab_cache_alloc(caches[cache]);
}
void game_free(cache_idx_t cache, void *ptr){
	bool ret = slab_cache_free(caches[cache], ptr);
	DEBUG_ASSERT(ret == true);
}
