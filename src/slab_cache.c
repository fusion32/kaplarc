#include "slab.h"
#include "def.h"
#include "log.h"
#include "system.h"

#include <string.h>

struct slab_cache{
	uint32 slab_slots;
	uint32 slab_stride;
	uint32 count;
	uint32 capacity;
	struct slab **slabs;
};

#define SC_INITIAL_CAPACITY 4
#define SC_GROW_STEP 4
static struct slab *slab_cache_grow(struct slab_cache *sc){
	struct slab *new_slab;
	struct slab **new_vec;
	uint32 new_cap;

	// check if we need to grow slabs vector
	if(sc->capacity == 0){
		ASSERT(sc->slabs == NULL);
		sc->capacity = SC_INITIAL_CAPACITY;
		sc->slabs = sys_malloc(SC_INITIAL_CAPACITY * sizeof(void*));
	}else if(sc->count >= sc->capacity){
		ASSERT(sc->slabs != NULL);
		new_cap = sc->capacity + SC_GROW_STEP;
		new_vec = sys_realloc(sc->slabs, new_cap * sizeof(void*));
		if(new_vec == NULL)
			return NULL;
		sc->capacity = new_cap;
		sc->slabs = new_vec;
	}

	// create new slab
	new_slab = slab_create(sc->slab_slots, sc->slab_stride);
	sc->slabs[sc->count] = new_slab;
	sc->count += 1;
	return new_slab;
}

#define PTR_ALIGNMENT		(sizeof(void*))
#define PTR_ALIGNMENT_MASK	(sizeof(void*) - 1)
struct slab_cache *slab_cache_create(uint32 slab_slots, uint32 slab_stride){
	struct slab_cache *sc;

	// check if stride has the minimum alignment requirement
	if((slab_stride & PTR_ALIGNMENT_MASK) != 0){
		LOG_WARNING("slab_create: ajusting stride to have"
			" the minimum alignment of %u", PTR_ALIGNMENT);
		slab_stride = (slab_stride + PTR_ALIGNMENT_MASK)
				& ~PTR_ALIGNMENT_MASK;
	}

	// allocate slab_cache and initialize it
	// NOTE: the first slab will be allocated on the
	// first allocation
	sc = sys_malloc(sizeof(struct slab_cache));
	sc->slab_slots = slab_slots;
	sc->slab_stride = slab_stride;
	sc->count = 0;
	sc->capacity = 0;
	sc->slabs = NULL;
	return sc;
}

void slab_cache_destroy(struct slab_cache *sc){
	if(sc->slabs != NULL){
		for(uint32 i = 0; i < sc->count; i += 1)
			slab_destroy(sc->slabs[i]);
		sys_free(sc->slabs);
	}
	sys_free(sc);
}

void *slab_cache_alloc(struct slab_cache *sc){
	struct slab *new_slab;
	void *ptr;

	// try to allocate from existing slabs
	for(uint32 i = 0; i < sc->count; i += 1){
		ptr = slab_alloc(sc->slabs[i]);
		if(ptr != NULL)
			return ptr;
	}

	// all slabs are full, create a new one
	new_slab = slab_cache_grow(sc);
	if(new_slab == NULL){
		LOG_WARNING("slab_cache_alloc: failed to grow");
		return NULL;
	}
	return slab_alloc(new_slab);
}

bool slab_cache_free(struct slab_cache *sc, void *ptr){
	for(uint32 i = 0; i < sc->count; i += 1){
		if(slab_free(sc->slabs[i], ptr))
			return true;
	}
	return false;
}

bool slab_cache_try_to_shrink(struct slab_cache *sc){
	uint32 next_pos = 0;
	bool ret = false;

	for(uint32 i = 0; i < sc->count; i += 1){
		if(slab_is_empty(sc->slabs[i])){
			slab_destroy(sc->slabs[i]);
			ret = true;
		}else{
			sc->slabs[next_pos] = sc->slabs[i];
			next_pos += 1;
		}
	}
	sc->count = next_pos;
	return ret;
}
