#include "packed_data.h"

#include "buffer_util.h"
#include <stdlib.h>
#include <string.h>

void pd_init(struct packed_data *pd, uint8 *buffer){
	pd->len = 0;
	pd->pos = 0;
	pd->base = buffer;
}

uint8 pd_peek_byte(struct packed_data *pd){
	return decode_u8(pd->base + pd->pos);
}

uint16 pd_peek_u16(struct packed_data *pd){
	return decode_u16_be(pd->base + pd->pos);
}

uint32 pd_peek_u32(struct packed_data *pd){
	return decode_u32_be(pd->base + pd->pos);
}

uint8 pd_get_byte(struct packed_data *pd){
	uint8 x = decode_u8(pd->base + pd->pos);
	pd->pos += 1;
	return x;
}

uint16 pd_get_u16(struct packed_data *pd){
	uint16 x = decode_u16_be(pd->base + pd->pos);
	pd->pos += 2;
	return x;
}

uint32 pd_get_u32(struct packed_data *pd){
	uint32 x = decode_u32_be(pd->base + pd->pos);
	pd->pos += 4;
	return x;
}

void pd_get_str(struct packed_data *pd, char *s, size_t maxlen){
	size_t len = pd_get_u16(pd);
	size_t copy_len = len;
	if(len == 0) return;
	if(maxlen > 0){
		if(copy_len >= maxlen)
			copy_len = maxlen - 1;
		memcpy(s, (pd->base + pd->pos), copy_len);
		s[copy_len] = 0;
	}
	pd->pos += len;
}

void pd_add_byte(struct packed_data *pd, uint8 x){
	encode_u8(pd->base + pd->pos, x);
	pd->pos += 1;
}

void pd_add_u16(struct packed_data *pd, uint16 x){
	encode_u16_be(pd->base + pd->pos, x);
	pd->pos += 2;
}

void pd_add_u32(struct packed_data *pd, uint32 x){
	encode_u32_be(pd->base + pd->pos, x);
	pd->pos += 4;
}

void pd_add_str(struct packed_data *pd, const char *s){
	pd_add_lstr(pd, s, strlen(s));
}

void pd_add_lstr(struct packed_data *pd, const char *s, size_t len){
	pd_add_u16(pd, (uint16)len);
	if(len == 0) return;
	memcpy(pd->base + pd->pos, s, len);
	pd->pos += len;
}

void pd_radd_byte(struct packed_data *pd, uint8 x){
	if(pd->pos < 1) return;
	pd->pos -= 1;
	encode_u8(pd->base + pd->pos, x);
}

void pd_radd_u16(struct packed_data *pd, uint16 x){
	if(pd->pos < 2) return;
	pd->pos -= 2;
	encode_u16_be(pd->base + pd->pos, x);
}

void pd_radd_u32(struct packed_data *pd, uint32 x){
	if(pd->pos < 4) return;
	pd->pos -= 4;
	encode_u32_be(pd->base + pd->pos, x);
}

void pd_radd_str(struct packed_data *pd, const char *s){
	pd_radd_lstr(pd, s, strlen(s));
}

void pd_radd_lstr(struct packed_data *pd, const char *s, size_t len){
	size_t total_len = len + 2;
	if(pd->pos < total_len) return;
	pd->pos -= total_len;
	encode_u16_be(pd->base + pd->pos, (uint16)len);
	if(len > 0) memcpy(pd->base + pd->pos + 2, s, len);
}
