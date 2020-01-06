#ifndef BITFIELD_H_
#define BITFIELD_H_

#include "def.h"

#define BITSET_COUNT(bits) ((bits + 31) >> 5)

bool bitset_is_set(uint32 *b, int bit);
void bitset_set(uint32 *b, int bit);
void bitset_clear(uint32 *b, int bit);
int bitset_first_set(uint32 *b, size_t count);
int bitset_first_clear(uint32 *b, size_t count);
bool bitset_is_all_set(uint32 *b, size_t count);
bool bitset_is_all_clear(uint32 *b, size_t count);
void bitset_set_all(uint32 *b, size_t count);
void bitset_clear_all(uint32 *b, size_t count);

#endif // BITFIELD_H_

