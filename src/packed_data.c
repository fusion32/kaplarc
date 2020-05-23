#include "packed_data.h"
#include "buffer_util.h"
#include <stdlib.h>
#include <string.h>

/*
 * READER
 */

uint8 data_peek_byte(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 1);
	return decode_u8(reader->ptr);
}

uint16 data_peek_u16(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 2);
	return decode_u16_le(reader->ptr);
}

uint32 data_peek_u32(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 4);
	return decode_u32_le(reader->ptr);
}

uint8 data_read_byte(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 1);
	uint8 ret = decode_u8(reader->ptr);
	reader->ptr += 1;
	return ret;
}

uint16 data_read_u16(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 2);
	uint16 ret = decode_u16_le(reader->ptr);
	reader->ptr += 2;
	return ret;
}

uint32 data_read_u32(struct data_reader *reader){
	DEBUG_ASSERT((reader->end - reader->ptr) >= 4);
	uint32 ret = decode_u32_le(reader->ptr);
	reader->ptr += 4;
	return ret;
}

void data_read_str(struct data_reader *reader, char *s, int maxlen){
	int len = data_read_u16(reader);
	int copy_len = len;
	DEBUG_ASSERT(len >= 0);
	DEBUG_ASSERT((reader->end - reader->ptr) >= len);
	if(len == 0){
		if(maxlen > 0) s[0] = 0;
		return;
	}
	if(maxlen > 0){
		if(copy_len >= maxlen)
			copy_len = maxlen - 1;
		memcpy(s, reader->ptr, copy_len);
		s[copy_len] = 0;
	}
	reader->ptr += len;
}

/*
 * WRITER
 */

void data_write_byte(struct data_writer *writer, uint8 val){
	DEBUG_ASSERT((writer->end - writer->ptr) >= 1);
	encode_u8(writer->ptr, val);
	writer->ptr += 1;
}

void data_write_u16(struct data_writer *writer, uint16 val){
	DEBUG_ASSERT((writer->end - writer->ptr) >= 2);
	encode_u16_le(writer->ptr, val);
	writer->ptr += 2;
}

void data_write_u32(struct data_writer *writer, uint32 val){
	DEBUG_ASSERT((writer->end - writer->ptr) >= 4);
	encode_u32_le(writer->ptr, val);
	writer->ptr += 4;
}

void data_write_str(struct data_writer *writer, const char *s){
	data_write_lstr(writer, s, (int)strlen(s));
}

void data_write_lstr(struct data_writer *writer, const char *s, int len){
	int total = len + 2;
	DEBUG_ASSERT((writer->end - writer->ptr) >= total);
	encode_u16_le(writer->ptr, (uint16)len);
	if(len > 0) memcpy(writer->ptr + 2, s, len);
	writer->ptr += total;
}
