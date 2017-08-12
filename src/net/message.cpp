
#include <string.h>
#include "message.h"

#ifdef __BIG_ENDIAN__
// the compiler should optimize these away
static inline uint16 swap_u16(uint16 x){
	return x;
}
static inline uint32 swap_u32(uint32 x){
	return x;
}
static inline uint64 swap_u64(uint64 x){
	return x;
}
#else
static inline uint16 swap_u16(uint16 x){
	return (x & 0xFF00) >> 8
		| (x & 0x00FF) << 8;
}
static inline uint32 swap_u32(uint32 x){
	return (x & 0xFF000000) >> 24
		| (x & 0x00FF0000) >> 8
		| (x & 0x0000FF00) << 8
		| (x & 0x000000FF) << 24;
}
static inline uint64 swap_u64(uint64 x){
	return (x & 0xFF00000000000000) >> 56
		| (x & 0x00FF000000000000) >> 40
		| (x & 0x0000FF0000000000) >> 24
		| (x & 0x000000FF00000000) >> 8
		| (x & 0x00000000FF000000) << 8
		| (x & 0x0000000000FF0000) << 24
		| (x & 0x000000000000FF00) << 40
		| (x & 0x00000000000000FF) << 56;
}
#endif

bool Message::try_acquire(void){
	return !busy.test_and_set(std::memory_order_acq_rel);
}

void Message::release(void){
	busy.clear(std::memory_order_relaxed);
}

uint8 Message::get_byte(void){
	uint8 val = *(uint8*)(buffer + readpos);
	readpos += 1;
	return val;
}

uint16 Message::get_u16(void){
	uint16 val = *(uint16*)(buffer + readpos);
	readpos += 2;
	return swap_u16(val);
}

uint32 Message::get_u32(void){
	uint32 val = *(uint32*)(buffer + readpos);
	readpos += 4;
	return swap_u32(val);
}

void Message::get_str(char *buf, uint16 buflen){
	uint16 len = get_u16();
	if(len == 0 || buflen == 0) return;
	if(len > buflen - 1)
		len = buflen - 1;

	memcpy(buf, (buffer + readpos), len);
	buf[len] = 0x00;
	readpos += len;
}

void Message::add_byte(uint8 val){
	*(uint8*)(buffer + readpos) = val;
	readpos += 1;
	length += 1;
}

void Message::add_u16(uint16 val){
	*(uint16*)(buffer + readpos) = swap_u16(val);
	readpos += 2;
	length += 2;
}

void Message::add_u32(uint32 val){
	*(uint32*)(buffer + readpos) = swap_u32(val);
	readpos += 4;
	length += 4;
}

void Message::add_str(const char *buf, uint16 buflen){
	if(buflen == 0) return;
	add_u16(buflen);
	memcpy((buffer + readpos), buf, buflen);
	readpos += buflen;
	length += buflen;
}

void Message::radd_byte(uint8 val){
	if((readpos - 1) < 0) return;
	readpos -= 1;
	*(uint8*)(buffer + readpos) = val;
	length += 1;
}

void Message::radd_u16(uint16 val){
	if((readpos - 2) < 0) return;
	readpos -= 2;
	*(uint16*)(buffer + readpos) = swap_u16(val);
	length += 2;
}

void Message::radd_u32(uint32 val){
	if((readpos - 4) < 0) return;
	readpos -= 4;
	*(uint32*)(buffer + readpos) = swap_u32(val);
	length += 4;
}

void Message::radd_str(const char *buf, uint16 buflen){
	if(buflen == 0) return;
	if((readpos - (2 + buflen)) < 0) return;
	readpos -= (2 + buflen);
	*(uint16*)(buffer + readpos) = swap_u16(buflen);
	memcpy((buffer + readpos + 2), buf, buflen);
	length += (2 + buflen);
}