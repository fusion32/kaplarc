#include "slab.h"

#define FREENODE_NEXT(ptr) (*(void**)(ptr))

// @REVIEW
// increasing the alignment past the cache line size wouldn't
// yield any improvements
#define MAX_ALIGNMENT		ARCH_CACHE_LINE_SIZE
#define PTR_ALIGNMENT		(sizeof(void*))
#define PTR_ALIGNMENT_MASK	(sizeof(void*) - 1)

struct slab *slab_create(uint32 slots, uint32 stride){
	uint32 capacity, mem_offset;
	uint32 alignment, alignment_mask;
	struct slab *s;

	// define object alignment
	if(IS_POWER_OF_TWO(stride))
		alignment = MIN(stride, MAX_ALIGNMENT);
	else
		alignment = PTR_ALIGNMENT;
	DEBUG_ASSERT(IS_POWER_OF_TWO(alignment));
	alignment_mask = alignment - 1;

	// stride must be a multiple of alignment
	if(stride & alignment_mask)
		stride = (stride + alignment_mask) & ~alignment_mask;

	// properly align the offset to memory
	mem_offset = sizeof(struct slab);
	if(mem_offset & alignment_mask)
		mem_offset = (mem_offset + alignment_mask) & ~alignment_mask;

	// allocate and initialize slab
	capacity = slots * stride;
	s = sys_aligned_malloc(mem_offset + capacity, alignment);
	s->stride = stride;
	s->capacity = capacity;
	s->offset = 0;
	s->freecount = 0;
	s->freelist = NULL;
	s->mem = OFFSET_POINTER(s, mem_offset);
	return s;
}

void slab_destroy(struct slab *s){
	sys_aligned_free(s);
}

void *slab_alloc(struct slab *s){
	void *ptr = NULL;
	if(s->freelist != NULL){
		DEBUG_ASSERT(s->freecount > 0);
		ptr = s->freelist;
		s->freelist = FREENODE_NEXT(s->freelist);
		s->freecount -= 1;
		return ptr;
	}
	if(s->offset < s->capacity){
		ptr = OFFSET_POINTER(s->mem, s->offset);
		s->offset += s->stride;
	}
	return ptr;
}

bool slab_free(struct slab *s, void *ptr){
	// check if ptr belongs to this slab
	if(ptr < s->mem || ptr >= OFFSET_POINTER(s->mem, s->capacity))
		return false;

	// return memory to slab
	uint32 last_offset = s->offset - s->stride;
	if(ptr == OFFSET_POINTER(s->mem, last_offset)){
		s->offset = last_offset;
	}else{
		FREENODE_NEXT(ptr) = s->freelist;
		s->freelist = ptr;
		s->freecount += 1;
	}
	return true;
}

bool slab_is_full(struct slab *s){
	return s->offset >= s->capacity && s->freecount == 0;
}

bool slab_is_empty(struct slab *s){
	return s->offset == (s->freecount * s->stride);
}