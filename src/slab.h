#ifndef SLAB_H_
#define SLAB_H_

#include "def.h"

struct slab;
struct slab *slab_create(uint slots, uint stride);
void slab_destroy(struct slab *s);
void *slab_alloc(struct slab *s);
bool slab_free(struct slab *s, void *ptr);
bool slab_is_full(struct slab *s);
bool slab_is_empty(struct slab *s);
struct slab **slab_next(struct slab *s);

#endif //SLAB_H_
