#ifndef MMBLOCK_H_
#define MMBLOCK_H_

#include "def.h"

struct mmblock;

struct mmblock *mmblock_create(long slots, long stride);
void mmblock_destroy(struct mmblock *blk);
void *mmblock_alloc(struct mmblock *blk);
void mmblock_free(struct mmblock *blk, void *ptr);
bool mmblock_contains(struct mmblock *blk, void *ptr);
void mmblock_report(struct mmblock *blk);

#endif //MMBLOCK_H_
