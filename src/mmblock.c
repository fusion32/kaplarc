#include "mmblock.h"
#include "log.h"
#include "thread.h"

#include <stdlib.h>
#include <stdio.h>

#define FREENODE_NEXT(ptr) (*(void**)(ptr))

struct mmblock{
	long stride;
	long capacity;
	long offset;
	void *base;
	void *freelist;
};

struct mmblock *mmblock_create(long slots, long stride){
	long capacity, offset;
	struct mmblock *blk;

	// properly align stride and data offset to have aligned allocations
	static const unsigned mask = sizeof(void*) - 1;
	if((stride & mask) != 0)
		stride = (stride + mask) & ~mask;

	offset = sizeof(struct mmblock);
	if((offset & mask) != 0)
		offset = (offset + mask) & ~mask;

	capacity = slots * stride;
	blk = malloc(offset + capacity);
	if(blk == NULL){
		LOG_ERROR("memory_block_create: out of memory");
		return NULL;
	}
	blk->stride = stride;
	blk->capacity = capacity;
	blk->offset = 0;
	blk->base = OFFSET_POINTER(blk, offset);
	blk->freelist = NULL;
	return blk;
}

void mmblock_destroy(struct mmblock *blk){
	free(blk);
}

void *mmblock_alloc(struct mmblock *blk){
	void *ptr = NULL;
	if(blk->freelist != NULL){
		ptr = blk->freelist;
		blk->freelist = FREENODE_NEXT(blk->freelist);
		return ptr;
	}
	if(blk->offset < blk->capacity){
		ptr = OFFSET_POINTER(blk->base, blk->offset);
		blk->offset += blk->stride;
	}
	return ptr;
}

void mmblock_free(struct mmblock *blk, void *ptr){
	// check if ptr belongs to this block
	if(!mmblock_contains(blk, ptr))
		return;

	// return memory to block
	if(ptr == OFFSET_POINTER(blk->base, blk->offset - blk->stride)){
		blk->offset -= blk->stride;
	}else{
		FREENODE_NEXT(ptr) = blk->freelist;
		blk->freelist = ptr;
	}
}

bool mmblock_contains(struct mmblock *blk, void *ptr){
	return (ptr >= blk->base) &&
		(ptr < OFFSET_POINTER(blk->base, blk->capacity));
}

void mmblock_report(struct mmblock *blk){
	void *ptr;
	LOG("memory block report:");
	LOG("\tstride = %ld", blk->stride);
	LOG("\tcapacity = %ld", blk->capacity);
	LOG("\toffset = %ld", blk->offset);
	LOG("\tbase: %p", blk->base);
	LOG("\tfreelist:");
	for(ptr = blk->freelist; ptr != NULL; ptr = *(void**)ptr)
		LOG("\t\t* %p", ptr);
}
