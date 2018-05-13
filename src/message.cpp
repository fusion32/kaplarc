#include "endian.h"
#include "message.h"
#include <string.h>
#include <vector>
#include <mutex>

Message::Message(size_t capacity_)
 : capacity(capacity_), readpos(0), length(0) {
	buffer = new uint8[capacity];
}

Message::~Message(void){
	delete[] buffer;
}

uint8 Message::peek_byte(void){
	uint8 val = *(uint8*)(buffer + readpos);
	return val;
}

uint16 Message::peek_u16(void){
	uint16 val = *(uint16*)(buffer + readpos);
	return u16_from_be(val);
}

uint32 Message::peek_u32(void){
	uint32 val = *(uint32*)(buffer + readpos);
	return u32_from_be(val);
}

uint8 Message::get_byte(void){
	uint8 val = *(uint8*)(buffer + readpos);
	readpos += 1;
	return val;
}

uint16 Message::get_u16(void){
	uint16 val = *(uint16*)(buffer + readpos);
	readpos += 2;
	return u16_from_be(val);
}

uint32 Message::get_u32(void){
	uint32 val = *(uint32*)(buffer + readpos);
	readpos += 4;
	return u32_from_be(val);
}

void Message::get_str(char *buf, uint16 buflen){
	// if `buflen` was not big enough, the message going
	// further would be corrupted if the original length
	// of the string was not cached
	uint16 len0 = get_u16();
	uint16 len = len0;
	if(len == 0 || buflen == 0) return;
	if(len > buflen - 1)
		len = buflen - 1;

	memcpy(buf, (buffer + readpos), len);
	buf[len] = 0x00;
	readpos += len0;
}

void Message::add_byte(uint8 val){
	*(uint8*)(buffer + readpos) = val;
	readpos += 1;
	length += 1;
}

void Message::add_u16(uint16 val){
	*(uint16*)(buffer + readpos) = u16_to_be(val);
	readpos += 2;
	length += 2;
}

void Message::add_u32(uint32 val){
	*(uint32*)(buffer + readpos) = u32_to_be(val);
	readpos += 4;
	length += 4;
}

void Message::add_str(const char *str){
	add_lstr(str, (uint16)strlen(str));
}

void Message::add_lstr(const char *buf, uint16 buflen){
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
	*(uint16*)(buffer + readpos) = u16_to_be(val);
	length += 2;
}

void Message::radd_u32(uint32 val){
	if((readpos - 4) < 0) return;
	readpos -= 4;
	*(uint32*)(buffer + readpos) = u32_to_be(val);
	length += 4;
}

void Message::radd_str(const char *str){
	radd_lstr(str, (uint16)strlen(str));
}

void Message::radd_lstr(const char *buf, uint16 buflen){
	if(buflen == 0) return;
	if((readpos - (2 + buflen)) < 0) return;
	readpos -= (2 + buflen);
	*(uint16*)(buffer + readpos) = u16_to_be(buflen);
	memcpy((buffer + readpos + 2), buf, buflen);
	length += (2 + buflen);
}
