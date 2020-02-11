#ifndef MEM_H_
#define MEM_H_
#include "def.h"

typedef enum cache_idx{
	CACHE64 = 0,
	CACHE128,
	CACHE256,
	CACHE512,
	CACHE1K,
	CACHE2K,
	CACHE4K,
	CACHE8K,
	CACHE16K,

	CACHE_FIRST = CACHE64,
	CACHE_LAST = CACHE16K,
	NUM_CACHES = CACHE_LAST + 1,
} cache_idx_t;

bool mem_init(void);
void mem_shutdown(void);

void *mem_alloc(cache_idx_t cache);
void mem_free(cache_idx_t cache, void *mem);

#endif //MEM_H_
