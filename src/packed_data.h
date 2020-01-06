#ifndef PACKED_DATA_BUFFER_H_
#define PACKED_DATA_BUFFER_H_

#include "def.h"

struct packed_data{
	size_t capacity;
	size_t length;
	size_t pos;
	uint8 *base;
};

// peek data
uint8	pd_peek_byte(struct packed_data *pd);
uint16	pd_peek_u16(struct packed_data *pd);
uint32	pd_peek_u32(struct packed_data *pd);

// get data
uint8	pd_get_byte(struct packed_data *pd);
uint16	pd_get_u16(struct packed_data *pd);
uint32	pd_get_u32(struct packed_data *pd);
void	pd_get_str(struct packed_data *pd, char *s, size_t maxlen);

// add data
void	pd_add_byte(struct packed_data *pd, uint8 x);
void	pd_add_u16(struct packed_data *pd, uint16 x);
void	pd_add_u32(struct packed_data *pd, uint32 x);
void	pd_add_str(struct packed_data *pd, const char *s);
void	pd_add_lstr(struct packed_data *pd, const char *s, size_t len);

// reverse add
void	pd_radd_byte(struct packed_data *pd, uint8 x);
void	pd_radd_u16(struct packed_data *pd, uint16 x);
void	pd_radd_u32(struct packed_data *pd, uint32 x);
void	pd_radd_str(struct packed_data *pd, const char *s);
void	pd_radd_lstr(struct packed_data *pd, const char *s, size_t len);

#endif //PACKED_DATA_BUFFER_H_
