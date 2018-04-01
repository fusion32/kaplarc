// C compatible source: don't change if you are
// going to break compatibility

#ifndef MEMBLOCK_H_
#define MEMBLOCK_H_

struct memblock;
struct memblock *memblock_create(long slots, long stride);
struct memblock *memblock_create1(void *data, size_t datalen, long stride);
void memblock_destroy(struct memblock *blk);
void *memblock_alloc(struct memblock *blk);
void memblock_free(struct memblock *blk, void *ptr);
bool memblock_contains(struct memblock *blk, void *ptr);
long memblock_stride(struct memblock *blk);

#endif //MEMBLOCK_H_
