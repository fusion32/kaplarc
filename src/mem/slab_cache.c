#include "slab.h"

#define OBJCACHE_SIZE 256
struct slab_cache{
	uint32 slab_slots;
	uint32 slab_stride;
	struct slab *full;
	struct slab *partial;
	struct slab *empty;
	int objcache_count;
	void *objcache[OBJCACHE_SIZE];
};

static int slab_list_destroy(struct slab *head){
	struct slab *tmp;
	struct slab *next = head;
	int ret = 0;
	while(next != NULL){
		tmp = next;
		next = tmp->next;
		slab_destroy(tmp);
		ret += 1;
	}
	return ret;
}

#define PTR_ALIGNMENT		(sizeof(void*))
#define PTR_ALIGNMENT_MASK	(sizeof(void*) - 1)
struct slab_cache *slab_cache_create(uint32 slab_slots, uint32 slab_stride){
	struct slab_cache *c;

	// check if stride has the minimum alignment requirement
	if((slab_stride & PTR_ALIGNMENT_MASK) != 0){
		LOG_WARNING("slab_cache_create: ajusting stride to have"
			" the minimum alignment of %u", PTR_ALIGNMENT);
		slab_stride = (slab_stride + PTR_ALIGNMENT_MASK)
				& ~PTR_ALIGNMENT_MASK;
	}

	// allocate control block and initialize it
	c = sys_malloc(sizeof(struct slab_cache));
	c->slab_slots = slab_slots;
	c->slab_stride = slab_stride;
	// the first slab will be created on the first allocation
	c->full = NULL;
	c->partial = NULL;
	c->empty = NULL;
	c->objcache_count = 0;
	return c;
}

void slab_cache_destroy(struct slab_cache *c){
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

void *slab_cache_alloc(struct slab_cache *c){
	struct slab *s;
	void *ptr;

	// check the obj cache
	if(c->objcache_count > 0)
		return c->objcache[--c->objcache_count];

	// try to allocate from partial slabs
	s = c->partial;
	while(s != NULL){
		ptr = slab_alloc(s);
		// this will only fail if the slab is full
		if(ptr != NULL)
			return ptr;

		// remove from partial list
		c->partial = s->next;
		// insert into full list
		s->next = c->full;
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
		c->empty = s->next;
		// insert into partial list
		s->next = c->partial;
		c->partial = s;

		return ptr;
	}

	// if there were no empty slabs, create new one
	// and insert it into the partial list
	s = slab_create(c->slab_slots, c->slab_stride);
	s->next = c->partial;
	c->partial = s;

	ptr = slab_alloc(s);
	ASSERT(ptr != NULL); // this shouldn't fail
	return ptr;
}

static void __ptr_sel_sort(void **arr, int lo, int hi){
	void *tmp;
	int i, j, min;
	for(i = lo; i < hi; i += 1){
		min = i;
		for(j = i + 1; j < hi; j += 1){
			if(arr[j] < arr[min])
				min = j;
		}
		tmp = arr[lo];
		arr[lo] = arr[min];
		arr[min] = tmp;
	}
}

static void __ptr_qsort(void **arr, int lo, int hi){
	void *pivot, *tmp;
	int i, j;

	// do a selection sort if the size of the partition
	// is smaller than a treshhold
	if((hi - lo) <= 32){
		__ptr_sel_sort(arr, lo, hi);
		return;
	}

	// partition array and recurse
	pivot = arr[hi];
	for(i = j = lo; j < hi; j += 1){
		if(arr[j] < pivot){
			tmp = arr[j];
			arr[j] = arr[i];
			arr[i] = tmp;
			i += 1;
		}
	}
	tmp = arr[hi];
	arr[hi] = arr[i];
	arr[i] = tmp;
	__ptr_qsort(arr, lo, i-1);
	__ptr_qsort(arr, i+1, hi);
}

static bool __slab_flush_objects(struct slab *s, void **arr, int *pcount){
	int count, i, j;

	// check if there is any object from this slab
	count = *pcount;
	for(i = 0; i < count && !slab_free(s, arr[i]); i += 1)
		continue;
	if(i >= count)
		return false;
	// return subsequent elements into the slab
	j = i;
	for(i += 1; i < count && slab_free(s, arr[i]); i += 1)
		continue;
	// update count
	*pcount -= (i - j);
	// shift the remainder of the array
	while(i < count){
		arr[j] = arr[i];
		j += 1; i += 1;
	}
	return true;
}

static void __slab_cache_flush_objcache(struct slab_cache *c){
	DEBUG_ASSERT(c->objcache_count > 0);
	struct slab **s, *tmp;
	void **objcache;
	int count;

	// first sort the objcache
	objcache = c->objcache;
	count = c->objcache_count;
	__ptr_qsort(objcache, 0, count-1);

	// NOTE: the following order is important because we move
	// slabs from the full list into the partial list and would
	// end up checking the same slabs more than once

	// return to partial slabs
	for(s = &c->partial; count > 0 && *s != NULL; s = &(*s)->next){
		if(__slab_flush_objects(*s, objcache, &count)){
			if(slab_is_empty(*s)){
				// remove from partial list
				tmp = *s;
				*s = tmp->next;
				// insert into empty list
				tmp->next = c->empty;
				c->empty = tmp;
			}
		}
	}

	// return to full slabs
	for(s = &c->full; count > 0 && *s != NULL; s = &(*s)->next){
		if(__slab_flush_objects(*s, objcache, &count)){
			// remove from full list
			tmp = *s;
			*s = tmp->next;
			if(slab_is_empty(tmp)){
				// insert into empty list
				tmp->next = c->empty;
				c->empty = tmp;
			}else{
				// insert into partial list
				tmp->next = c->partial;
				c->partial = tmp;
			}
		}
	}
	ASSERT(count == 0 && "invalid slab_cache_free");
	c->objcache_count = 0;
}

void slab_cache_free(struct slab_cache *c, void *ptr){
	// if the obj cache is full, flush the cache back
	// into the slabs
	if(c->objcache_count >= OBJCACHE_SIZE)
		__slab_cache_flush_objcache(c);

	// now add the current object into the objcache
	c->objcache[c->objcache_count++] = ptr;
}

int slab_cache_shrink(struct slab_cache *c){
	int ret;
	if(c->objcache_count > 0)
		__slab_cache_flush_objcache(c);
	ret = slab_list_destroy(c->empty);
	c->empty = NULL;
	return ret;
}
