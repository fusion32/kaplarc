#include "common.h"

struct mem_arena_block{
	struct mem_arena_block *next_block;
	uint8 *mem_prev;
	uint8 *mem_ptr;
	uint8 *mem_end;
	uint8 mem_start[];
};

void mem_arena_cleanup(struct mem_arena *arena){
	struct mem_arena_block *tmp, *next;
	next = arena->head;
	while(next != NULL){
		tmp = next;
		next = next->next_block;
		kpl_free(tmp);
	}
	arena->head = NULL;
}

static
struct mem_arena_block *mem_arena_block_create(size_t block_size){
	struct mem_arena_block *blk;
	PTR_ALIGN(block_size);
	blk = kpl_malloc(sizeof(struct mem_arena_block) + block_size);
	blk->next_block = NULL;
	blk->mem_prev = NULL;
	blk->mem_ptr = blk->mem_start;
	blk->mem_end = blk->mem_start + block_size;
	return blk;
}

static
void *mem_arena_block_alloc(struct mem_arena_block *blk, size_t size){
	uint8 *ptr = blk->mem_ptr;
	uint8 *nextptr = blk->mem_ptr + size;
	if(nextptr >= blk->mem_end)
		return NULL;
	blk->mem_prev = ptr;
	blk->mem_ptr = nextptr;
	return ptr;
}

void *mem_arena_alloc(struct mem_arena *arena, size_t size){
	struct mem_arena_block *blk;
	void *ptr;
	// pointer align size
	PTR_ALIGN(size);
	// disallow allocations larger than half the block's size
	if(size > arena->block_size/2)
		return NULL;
	// create first block if there is none
	if(arena->head == NULL){
		blk = mem_arena_block_create(arena->block_size);
		arena->head = blk;
	}else{
		blk = arena->head;
	}
	// if we fail to allocate from head, create a new block
	// and allocate from it
	ptr = mem_arena_block_alloc(blk, size);
	if(ptr == NULL){
		blk = mem_arena_block_create(arena->block_size);
		blk->next_block = arena->head;
		arena->head = blk;
		ptr = mem_arena_block_alloc(blk, size);
		ASSERT(ptr != NULL);
	}
	return ptr;
}

void mem_arena_rollback(struct mem_arena *arena){
	struct mem_arena_block *blk = arena->head;
	DEBUG_CHECK(blk != NULL && blk->mem_prev != NULL,
		"mem_arena_rollback: nothing to rollback"
		" (it's probably a bug)");
	if(blk != NULL && blk->mem_prev != NULL){
		blk->mem_ptr = blk->mem_prev;
		blk->mem_prev = NULL;
	}
}

void mem_arena_reset(struct mem_arena *arena){
	struct mem_arena_block *tmp, *next;
	struct mem_arena_block *head = arena->head;
	if(!head)
		return;
	// release all blocks after the head
	if(head->next_block){
		next = head->next_block;
		while(next != NULL){
			tmp = next;
			next = next->next_block;
			kpl_free(tmp);
		}
		head->next_block = NULL;
	}
	// reset head
	head->mem_prev = NULL;
	head->mem_ptr = head->mem_start;
}
