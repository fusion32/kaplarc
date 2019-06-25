#ifndef BITFIELD_H_
#define BITFIELD_H_

#include "def.h"
#include <stdlib.h>
#include <string.h>

#define BITSET_SLOT(bit) ((bit) >> 3)
#define BITSET_MASK(bit) (1 << ((bit) & 7))

template<int N>
class BitSet{
private:
	static_assert((N & 7) == 0,
		"BitSet<N> requires N to be a multiple of 8");
	static constexpr size_t bytes_ = N >> 3;
	uint8 data_[bytes_];

public:
	BitSet &operator=(const BitSet &other){
		memcpy(data_, other.data_, bytes_);
		return *this;
	}

	template<int M>
	BitSet &operator=(const BitSet<M> &other){
		DEBUG_LOG("BitSet assignment mismatch");
		if(bytes_ < other.bytes_){
			memcpy(data_, other.data_, bytes_);
		}else{
			memcpy(data_, other.data_, other.bytes_);
			for(int i = other.bytes_; i < bytes_; i += 1)
				data_[i] = 0x00;
		}
		return *this;
	}

	bool is_set(int bit){
		return data_[BITSET_SLOT(bit)] & BITSET_MASK(bit);
	}
	void set(int bit){
		data_[BITSET_SLOT(bit)] |= BITSET_MASK(bit);
	}
	void clear(int bit){
		data_[BITSET_SLOT(bit)] &= ~BITSET_MASK(bit);
	}

	int first_set(void){
		static const int aux[16] = {
			4, 0, 1, 0,
			2, 0, 1, 0,
			3, 0, 1, 0,
			2, 0, 1, 0};
		int bit, byte;
		for(int i = 0; i < bytes_; i += 1){
			if(data_[i] != 0x00){
				bit = i << 3;
				byte = data_[i];
				if((byte & 0x0F) == 0x00){
					bit += 4;
					byte >>= 4;
				}
				return bit + aux[byte & 0x0F];
			}
		}
		return -1;
	}
	int first_clear(void){
		static const int aux[16] = {
			0, 1, 0, 2,
			0, 1, 0, 3,
			0, 1, 0, 2,
			0, 1, 0, 4};
		int bit, byte;
		for(int i = 0; i < bytes_; i += 1){
			if(data_[i] != 0xFF){
				bit = i << 3;
				byte = data_[i];
				if((byte & 0x0F) == 0x0F){
					bit += 4;
					byte >>= 4;
				}
				return bit + aux[byte & 0x0F];
			}
		}
		return -1;
	}

	bool is_all_set(void){
		for(int i = 0; i < bytes_; i += 1){
			if(data_[i] != 0xFF)
				return false;
		}
		return true;
	}
	bool is_all_clear(void){
		for(int i = 0; i < bytes_; i += 1){
			if(data_[i] != 0x00)
				return false;
		}
		return true;
	}

	void set_all(void){
		for(int i = 0; i < bytes_; i += 1)
			data_[i] = 0xFF;
	}
	void clear_all(void){
		for(int i = 0; i < bytes_; i += 1)
			data_[i] = 0x00;
	}
};

#endif // BITFIELD_H_

