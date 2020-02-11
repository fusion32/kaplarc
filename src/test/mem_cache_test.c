#include "../def.h"
#ifdef BUILD_TEST

#include "../log.h"
#include "../mem_cache.h"

#define SLAB_SLOTS 1024
#define SLAB_STRIDE sizeof(struct element)

struct element{
	void *a;
	void *b;
	void *c;
	void *d;
};

#define ALLOC_COUNT (SLAB_SLOTS * 4)
static struct mem_cache *cache;
static struct element *elements[ALLOC_COUNT];

bool mem_cache_test(void){
	bool ret = true;

	cache = mem_cache_create(SLAB_SLOTS, SLAB_STRIDE);
	if(cache == NULL){
		LOG_ERROR("mem_cache_test: failed to create cache");
		return false;
	}

	for(int i = 0; i < ALLOC_COUNT; i += 1){
		elements[i] = mem_cache_alloc(cache);
		if(elements[i] == NULL){
			LOG_ERROR("mem_cache_test: allocation failed");
			ret = false;
			break;
		}
	}

	if(ret){
		for(int i = ALLOC_COUNT/2; i < ALLOC_COUNT; i += 1){
			if(!mem_cache_free(cache, elements[i])){
				LOG_ERROR("mem_cache_test: failed to free element");
				ret = false;
				break;
			}
		}
	}

	if(ret){
		ret = mem_cache_shrink(cache);
		if(!ret) LOG_ERROR("mem_cache_test: failed to shrink");
	}

	mem_cache_destroy(cache);
	return ret;
}

#endif
