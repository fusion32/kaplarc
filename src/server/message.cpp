#include "../buffer_util.h"
#include "../system.h"
#include "message.h"
#include <stdlib.h>
#include <string.h>

/*
 * NOTE: the usual structure alignment/padding
 * should ensure the alignment of `buffer` without
 * the need of wild sorcery
 */
static constexpr size_t buffer_offset = sizeof(Message);

/*
 * NOTE: although the message doesn't need any
 * internal alignment (see above), when there is
 * an array of them, each element must be aligned
 * to alignof(Message) at least
 */
static constexpr size_t alignment = alignof(Message);
static constexpr size_t alignment_mask = alignment - 1;
static size_t align_up(size_t sz){
	return (sz + alignment_mask) & ~alignment_mask;
}

static size_t align_down(size_t sz){
	return sz & ~alignment_mask;
}

size_t message_total_size(size_t capacity){
	return align_up(buffer_offset + capacity);
}

Message *message_create(size_t total_size){
	Message *msg;

	total_size = align_up(total_size);
	if(total_size < buffer_offset)
		return nullptr;
	msg = (Message*)sys_aligned_alloc(
		alignment, total_size);
	if(msg == nullptr)
		return nullptr;
	msg->capacity = total_size - buffer_offset;
	msg->length = 0;
	msg->readpos = 0;
	return msg;
}

Message *message_create_with_capacity(size_t capacity){
	return message_create(buffer_offset + capacity);
}

Message *message_takeon(void *mem, size_t mem_size){
	Message *msg;
	uintptr addr;
	
	addr = (uintptr)mem;
	mem_size = align_down(mem_size);
	if(addr == 0
	  || addr & alignment
	  || mem_size < buffer_offset)
		return nullptr;
	msg = (Message*)mem;
	msg->capacity = mem_size - buffer_offset;
	msg->length = 0;
	msg->readpos = 0;
	return msg;
}

void message_destroy(Message *msg){
	sys_aligned_free(msg);
}

uint8 *Message::ptr(void){
	return buffer + readpos;
}

uint8 Message::peek_byte(void){
	return decode_u8(buffer + readpos);
}

uint16 Message::peek_u16(void){
	return decode_u16_be(buffer + readpos);
}

uint32 Message::peek_u32(void){
	return decode_u32_be(buffer + readpos);
}

uint8 Message::get_byte(void){
	uint8 val = decode_u8(buffer + readpos);
	readpos += 1;
	return val;
}

uint16 Message::get_u16(void){
	uint16 val = decode_u16_be(buffer + readpos);
	readpos += 2;
	return val;
}

uint32 Message::get_u32(void){
	uint32 val = decode_u32_be(buffer + readpos);
	readpos += 4;
	return val;
}

void Message::get_str(char *buf, size_t buflen){
	// if `buflen` was not big enough, the message going
	// further would be corrupted if the original length
	// of the string was not cached
	size_t len0 = get_u16();
	size_t len = len0;
	if(len == 0 || buflen == 0) return;
	if(len > buflen - 1)
		len = buflen - 1;
	memcpy(buf, (buffer + readpos), len);
	buf[len] = 0x00;
	readpos += len0;
}

void Message::add_byte(uint8 val){
	encode_u8(buffer + readpos, val);
	readpos += 1;
	length += 1;
}

void Message::add_u16(uint16 val){
	encode_u16_be(buffer + readpos, val);
	readpos += 2;
	length += 2;
}

void Message::add_u32(uint32 val){
	encode_u32_be(buffer + readpos, val);
	readpos += 4;
	length += 4;
}

void Message::add_str(const char *str){
	add_lstr(str, strlen(str));
}

void Message::add_lstr(const char *buf, size_t buflen){
	add_u16((uint16)buflen);
	if(buflen == 0) return;
	memcpy((buffer + readpos), buf, buflen);
	readpos += buflen;
	length += buflen;
}

void Message::radd_byte(uint8 val){
	if(readpos < 1) return;
	readpos -= 1;
	encode_u8(buffer + readpos, val);
	length += 1;
}

void Message::radd_u16(uint16 val){
	if(readpos < 2) return;
	readpos -= 2;
	encode_u16_be(buffer + readpos, val);
	length += 2;
}

void Message::radd_u32(uint32 val){
	if(readpos < 4) return;
	readpos -= 4;
	encode_u32_be(buffer + readpos, val);
	length += 4;
}

void Message::radd_str(const char *str){
	radd_lstr(str, strlen(str));
}

void Message::radd_lstr(const char *buf, size_t buflen){
	size_t len = buflen + 2;
	if(readpos < len) return;
	readpos -= len;
	encode_u16_be(buffer + readpos, (uint16)buflen);
	if(buflen > 0)
		memcpy((buffer + readpos + 2), buf, buflen);
	length += len;
}
