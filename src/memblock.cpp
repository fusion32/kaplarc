// C compatible file: don't change if you are
// going to break compatibility

#include "def.h"
#include "log.h"
#include "memblock.h"

#include <stdlib.h>

struct memblock{
	long stride;
	long capacity;
	long offset;
	void *freelist;
	void *base;
};

struct memblock *memblock_create(long slots, long stride){
	static const long mask = sizeof(void*) - 1;
	struct memblock *blk;
	long capacity, offset;

	// align offset
	offset = sizeof(struct memblock);
	if((offset & mask) != 0)
		offset = (offset + mask) & ~mask;

	//align stride
	if((stride & mask) != 0)
		stride = (stride + mask) & ~mask;

	// create block
	capacity = slots * stride;
	blk = (struct memblock*)malloc(offset + capacity);
	if(!blk){
		LOG_ERROR("memblock_create: out of memory");
		return NULL;
	}
	blk->stride = stride;
	blk->capacity = capacity;
	blk->offset = 0;
	blk->freelist = NULL;
	blk->base = (char*)blk + offset;
	return blk;
}

struct memblock *memblock_create1(void *data, size_t datalen, long stride){
	static const long mask = sizeof(void*) - 1;
	struct memblock *blk;
	uintptr base;

	// align data
	base = (uintptr)data;
	if((base & mask) != 0){
		datalen -= (base & mask);
		base = (base + mask) & ~mask;
	}

	// align stride
	if((stride & mask) != 0)
		stride = (stride + mask) & ~mask;

	// create block
	blk = (struct memblock*)malloc(sizeof(struct memblock));
	if(!blk){
		LOG_ERROR("memblock_create1: out of memory");
		return NULL;
	}
	blk->stride = stride;
	blk->capacity = (long)datalen;
	blk->offset = 0;
	blk->freelist = NULL;
	blk->base = (void*)base;
	return blk;
}

void memblock_destroy(struct memblock *blk){
	free(blk);
}

void *memblock_alloc(struct memblock *blk){
	void *ptr = NULL;
	if(blk->freelist != NULL){
		ptr = blk->freelist;
		blk->freelist = *(void**)blk->freelist;
	}else if(blk->offset < blk->capacity){
		ptr = (void*)((char*)blk->base + blk->offset);
		blk->offset += blk->stride;
	}
	return ptr;
}

void memblock_free(struct memblock *blk, void *ptr){
	if(!memblock_contains(blk, ptr))
		return;

	if(ptr == (void*)((char*)blk->base + blk->offset - blk->stride)){
		blk->offset -= blk->stride;
	}else{
		*(void**)ptr = blk->freelist;
		blk->freelist = ptr;
	}
}

bool memblock_contains(struct memblock *blk, void *ptr){
	return ptr >= blk->base &&
		ptr < (void*)((char*)blk->base + blk->capacity);
}

long memblock_stride(struct memblock *blk){
	return blk->stride;
}
