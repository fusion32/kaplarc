#ifndef BITFIELD_H_
#define BITFIELD_H_

#include "def.h"
#include <stdlib.h>
#include <string.h>

#define SLOT(bit) ((bit) >> 5)		// ((bit) / 32)
#define MASK(bit) (1 << ((bit) & 31))	// (1 << ((bit) % 32))

bool bitset_is_set(uint32 *b, int bit){
	return b[SLOT(bit)] & MASK(bit);
}

void bitset_set(uint32 *b, int bit){
	b[SLOT(bit)] |= MASK(bit);
}

void bitset_clear(uint32 *b, int bit){
	b[SLOT(bit)] &= ~MASK(bit);
}


int bitset_first_set(uint32 *b, size_t count){
	static const int aux[16] = {
		4, 0, 1, 0,
		2, 0, 1, 0,
		3, 0, 1, 0,
		2, 0, 1, 0};
	int bit;
	uint32 val;
	for(int i = 0; i < count; i += 1){
		if(b[i] != 0x00){
			bit = i << 5;
			val = b[i];
			if((val & 0xFFFF) == 0){
				bit += 16;
				val >>= 16;
			}
			if((val & 0xFF) == 0){
				bit += 8;
				val >>= 8;
			}
			if((val & 0x0F) == 0){
				bit += 4;
				val >>= 4;
			}
			return bit + aux[val & 0x0F];
		}
	}
	return -1;
}

int bitset_first_clear(uint32 *b, size_t count){
	static const int aux[16] = {
		0, 1, 0, 2,
		0, 1, 0, 3,
		0, 1, 0, 2,
		0, 1, 0, 4};
	int bit;
	uint32 val;
	for(int i = 0; i < count; i += 1){
		if(b[i] != 0xFFFFFFFF){
			bit = i << 5;
			val = b[i];
			if((val & 0xFFFF) == 0xFFFF){
				bit += 16;
				val >>= 16;
			}
			if((val & 0xFF) == 0xFF){
				bit += 8;
				val >>= 8;
			}
			if((val & 0x0F) == 0x0F){
				bit += 4;
				val >>= 4;
			}
			return bit + aux[val & 0x0F];
		}
	}
	return -1;
}

bool bitset_is_all_set(uint32 *b, size_t count){
	for(int i = 0; i < count; i += 1){
		if(b[i] != 0xFFFFFFFF)
			return false;
	}
	return true;
}
bool bitset_is_all_clear(uint32 *b, size_t count){
	for(int i = 0; i < count; i += 1){
		if(b[i] != 0x00)
			return false;
	}
	return true;
}

void bitset_set_all(uint32 *b, size_t count){
	for(int i = 0; i < count; i += 1)
		b[i] = 0xFFFFFFFF;
}

void bitset_clear_all(uint32 *b, size_t count){
	for(int i = 0; i < count; i += 1)
		b[i] = 0x00;
}


#endif // BITFIELD_H_

