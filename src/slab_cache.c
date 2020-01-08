// the slab cache's purpose is to allow faster allocations of
// common server constructs but as this

#include "slab.h"
#include "def.h"
#include "log.h"
#include "system.h"

#include <string.h>

struct slab_cache{
	uint32 slab_slots;
	uint32 slab_stride;
	struct slab *full;
	struct slab *partial;
	struct slab *empty;
};

static int slab_list_destroy(struct slab *head){
	struct slab *tmp;
	struct slab *next = head;
	int ret = 0;
	while(next != NULL){
		tmp = next;
		next = *slab_next(tmp);
		slab_destroy(tmp);
		ret += 1;
	}
	return ret;
}

#define PTR_ALIGNMENT		(sizeof(void*))
#define PTR_ALIGNMENT_MASK	(sizeof(void*) - 1)
struct slab_cache *slab_cache_create(uint slab_slots, uint slab_stride){
	struct slab_cache *sc;

	// check if stride has the minimum alignment requirement
	if((slab_stride & PTR_ALIGNMENT_MASK) != 0){
		LOG_WARNING("slab_cache_create: ajusting stride to have"
			" the minimum alignment of %u", PTR_ALIGNMENT);
		slab_stride = (slab_stride + PTR_ALIGNMENT_MASK)
				& ~PTR_ALIGNMENT_MASK;
	}

	// allocate control block and initialize it
	sc = sys_malloc(sizeof(struct slab_cache));
	sc->slab_slots = slab_slots;
	sc->slab_stride = slab_stride;
	// the first slab will be created on the first allocation
	sc->full = NULL;
	sc->partial = NULL;
	sc->empty = NULL;
	return sc;
}

void slab_cache_destroy(struct slab_cache *sc){
	// destroy slabs
	slab_list_destroy(sc->full);
	sc->full = NULL;
	slab_list_destroy(sc->partial);
	sc->partial = NULL;
	slab_list_destroy(sc->empty);
	sc->empty = NULL;

	// release control block
	sys_free(sc);
}

void *slab_cache_alloc(struct slab_cache *sc){
	struct slab *s;
	void *ptr;

	// try to allocate from partial slabs
	s = sc->partial;
	while(s != NULL){
		ptr = slab_alloc(s);
		// this will only fail if the slab is full
		if(ptr != NULL)
			return ptr;

		// remove from partial list
		sc->partial = *slab_next(s);
		// insert into full list
		*slab_next(s) = sc->full;
		sc->full = s;
		// next partial slab
		s = sc->partial;
	}

	// try to allocate from empty slabs
	s = sc->empty;
	if(s != NULL){
		ptr = slab_alloc(s);
		ASSERT(ptr != NULL); // this shouldn't fail

		// remove from empty list
		sc->empty = *slab_next(s);
		// insert into partial list
		*slab_next(s) = sc->partial;
		sc->partial = s;

		return ptr;
	}

	// if there were no empty slabs, create new one
	// and insert it into the partial list
	s = slab_create(sc->slab_slots, sc->slab_stride);
	*slab_next(s) = sc->partial;
	sc->partial = s;

	ptr = slab_alloc(s);
	ASSERT(ptr != NULL); // this shouldn't fail
	return ptr;
}

bool slab_cache_free(struct slab_cache *sc, void *ptr){
	struct slab **s, *tmp;

	// try to free from full list
	for(s = &sc->full; *s != NULL; s = slab_next(*s)){
		if(slab_free(*s, ptr)){
			// remove from full list
			tmp = *s;
			*s = *slab_next(tmp);
			if(slab_is_empty(tmp)){
				// insert into empty list
				*slab_next(tmp) = sc->empty;
				sc->empty = tmp;
			}else{
				// insert into partial list
				*slab_next(tmp) = sc->partial;
				sc->partial = tmp;
			}
			return true;
		}
	}

	// try to free from partial list
	for(s = &sc->partial; *s != NULL; s = slab_next(*s)){
		if(slab_free(*s, ptr)){
			if(slab_is_empty(*s)){
				// remove from partial list
				tmp = *s;
				*s = *slab_next(tmp);
				// insert into empty list
				*slab_next(tmp) = sc->empty;
				sc->empty = tmp;
			}
			return true;
		}
	}

	return false;
}

int slab_cache_shrink(struct slab_cache *sc){
	int ret = slab_list_destroy(sc->empty);
	sc->empty = NULL;
	return ret;
}
