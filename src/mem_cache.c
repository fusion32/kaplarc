#include "slab.h"
#include "def.h"
#include "log.h"
#include "system.h"

#include <string.h>

struct mem_cache{
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
struct mem_cache *mem_cache_create(uint slab_slots, uint slab_stride){
	struct mem_cache *c;

	// check if stride has the minimum alignment requirement
	if((slab_stride & PTR_ALIGNMENT_MASK) != 0){
		LOG_WARNING("mem_cache_create: ajusting stride to have"
			" the minimum alignment of %u", PTR_ALIGNMENT);
		slab_stride = (slab_stride + PTR_ALIGNMENT_MASK)
				& ~PTR_ALIGNMENT_MASK;
	}

	// allocate control block and initialize it
	c = sys_malloc(sizeof(struct mem_cache));
	c->slab_slots = slab_slots;
	c->slab_stride = slab_stride;
	// the first slab will be created on the first allocation
	c->full = NULL;
	c->partial = NULL;
	c->empty = NULL;
	return c;
}

void mem_cache_destroy(struct mem_cache *c){
	// destroy slabs
	slab_list_destroy(c->full);
	c->full = NULL;
	slab_list_destroy(c->partial);
	c->partial = NULL;
	slab_list_destroy(c->empty);
	c->empty = NULL;

	// release control block
	sys_free(c);
}

void *mem_cache_alloc(struct mem_cache *c){
	struct slab *s;
	void *ptr;

	// try to allocate from partial slabs
	s = c->partial;
	while(s != NULL){
		ptr = slab_alloc(s);
		// this will only fail if the slab is full
		if(ptr != NULL)
			return ptr;

		// remove from partial list
		c->partial = *slab_next(s);
		// insert into full list
		*slab_next(s) = c->full;
		c->full = s;
		// next partial slab
		s = c->partial;
	}

	// try to allocate from empty slabs
	s = c->empty;
	if(s != NULL){
		ptr = slab_alloc(s);
		ASSERT(ptr != NULL); // this shouldn't fail

		// remove from empty list
		c->empty = *slab_next(s);
		// insert into partial list
		*slab_next(s) = c->partial;
		c->partial = s;

		return ptr;
	}

	// if there were no empty slabs, create new one
	// and insert it into the partial list
	s = slab_create(c->slab_slots, c->slab_stride);
	*slab_next(s) = c->partial;
	c->partial = s;

	ptr = slab_alloc(s);
	ASSERT(ptr != NULL); // this shouldn't fail
	return ptr;
}

bool mem_cache_free(struct mem_cache *c, void *ptr){
	struct slab **s, *tmp;

	// try to free from full list
	for(s = &c->full; *s != NULL; s = slab_next(*s)){
		if(slab_free(*s, ptr)){
			// remove from full list
			tmp = *s;
			*s = *slab_next(tmp);
			if(slab_is_empty(tmp)){
				// insert into empty list
				*slab_next(tmp) = c->empty;
				c->empty = tmp;
			}else{
				// insert into partial list
				*slab_next(tmp) = c->partial;
				c->partial = tmp;
			}
			return true;
		}
	}

	// try to free from partial list
	for(s = &c->partial; *s != NULL; s = slab_next(*s)){
		if(slab_free(*s, ptr)){
			if(slab_is_empty(*s)){
				// remove from partial list
				tmp = *s;
				*s = *slab_next(tmp);
				// insert into empty list
				*slab_next(tmp) = c->empty;
				c->empty = tmp;
			}
			return true;
		}
	}

	return false;
}

int mem_cache_shrink(struct mem_cache *c){
	int ret = slab_list_destroy(c->empty);
	c->empty = NULL;
	return ret;
}
