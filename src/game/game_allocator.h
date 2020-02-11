#ifndef GAME_ALLOCATOR_H_
#define GAME_ALLOCATOR_H_
#include "../def.h"

typedef enum cache_idx{
	CACHE_64 = 0,
	CACHE_128,
	CACHE_256,
	CACHE_512,
	CACHE_1024,

	CACHE_FIRST = CACHE_64,
	CACHE_LAST = CACHE_1024,
	NUM_CACHES = CACHE_LAST + 1,

	CACHE_LOGIN_BUFFER = CACHE_256,
} cache_idx_t;

bool game_allocator_init(void);
void game_allocator_shutdown(void);

void *game_alloc(cache_idx_t cache);
void game_free(cache_idx_t cache, void *mem);

#endif //GAME_ALLOCATOR_H_
