
// @REVIEW: this whole reader thing might be unnecessary

#ifndef KAPLAR_PACKED_DATA_H_
#define KAPLAR_PACKED_DATA_H_ 1
#include "common.h"

struct data_reader{
	uint8 *ptr;
	uint8 *end;
};

struct data_writer{
	uint8 *ptr;
	uint8 *base;
	uint8 *end;
};

// reader
static INLINE void
data_reader_init(struct data_reader *reader, uint8 *data, uint32 datalen){
	reader->ptr = data;
	reader->end = data + datalen;
}

uint8 data_peek_byte(struct data_reader *reader);
uint16 data_peek_u16(struct data_reader *reader);
uint32 data_peek_u32(struct data_reader *reader);

uint8 data_read_byte(struct data_reader *reader);
uint16 data_read_u16(struct data_reader *reader);
uint32 data_read_u32(struct data_reader *reader);
void data_read_str(struct data_reader *reader, char *s, int maxlen);

// writer
static INLINE void
data_writer_init(struct data_writer *writer, uint8 *data, uint32 maxlen){
	writer->ptr = data;
	writer->base = data;
	writer->end = data + maxlen;
}

void data_write_byte(struct data_writer *writer, uint8 val);
void data_write_u16(struct data_writer *writer, uint16 val);
void data_write_u32(struct data_writer *writer, uint32 val);
void data_write_str(struct data_writer *writer, const char *s);
void data_write_lstr(struct data_writer *writer, const char *s, int len);

#endif //KAPLAR_PACKED_DATA_H_
