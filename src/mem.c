#include "mem.h"
#include "mem_cache.h"
#include "thread.h"

static mutex_t mtx;
static struct {
	uint slots;
	uint stride;
	struct mem_cache *cache;
} cache_table[] = {
	// ~8K for small allocations
	[CACHE64] = {128, 64, NULL},
	[CACHE128] = {64, 128, NULL},
	[CACHE256] = {32, 256, NULL},
	[CACHE512] = {16, 512, NULL},
	[CACHE1K] = {8, 1024, NULL},

	// ~32K for medium allocations
	[CACHE2K] = {16, 2048, NULL},
	[CACHE4K] = {8, 4096, NULL},

	// ~128K for large allocations
	[CACHE8K] = {16, 8192, NULL},
	[CACHE16K] = {8, 16384, NULL},
};

bool mem_init(void){
	mutex_init(&mtx);
	for(int i = 0; i < NUM_CACHES; i += 1){
		cache_table[i].cache = mem_cache_create(
			cache_table[i].slots, cache_table[i].stride);
		ASSERT(cache_table[i].cache != NULL);
	}
	return true;
}

void mem_shutdown(void){
	for(int i = 0; i < NUM_CACHES; i += 1)
		mem_cache_destroy(cache_table[i].cache);
	mutex_destroy(&mtx);
}

void *mem_alloc(cache_idx_t cache){
	void *ptr;
	mutex_lock(&mtx);
	ptr = mem_cache_alloc(cache_table[cache].cache);
	mutex_unlock(&mtx);
	DEBUG_ASSERT(ptr != NULL);
	return ptr;
}
void mem_free(cache_idx_t cache, void *ptr){
	bool ret;
	mutex_lock(&mtx);
	ret = mem_cache_free(cache_table[cache].cache, ptr);
	mutex_unlock(&mtx);
	DEBUG_ASSERT(ret);
}
