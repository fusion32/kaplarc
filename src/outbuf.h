#ifndef KAPLAR_OUTBUF_H_
#define KAPLAR_OUTBUF_H_ 1

#include "common.h"

// outbuf interface
//	NOTES:
//	- The outbuf interface is a simple linked list of
//	available free output buffers. If there is none
//	available, one is allocated and returned.
//	- There is no internal tracking of the buffers so
//	they must be manually managed.
//	- acquire and release are thread safe.

// @TODO: instead of having a max number of idle outbufs,
// try to make a statistic and calculate the average used
// outbufs per frame
#define MAX_IDLE_OUTBUFS 2048

#define MAX_OUTBUF_LEN (16384 - 2*sizeof(void*))
struct outbuf{
	struct outbuf *next;
	uint8 *ptr;
	uint8 base[MAX_OUTBUF_LEN];
};

bool outbuf_init(void);
void outbuf_shutdown(void);
struct outbuf *outbuf_acquire(void);
void outbuf_release(struct outbuf *buf);

INLINE uint8 *outbuf_data(struct outbuf *buf){
	return &buf->base[0];
}
INLINE uint32 outbuf_len(struct outbuf *buf){
	return (uint32)(buf->ptr - buf->base);
}
void outbuf_write_byte(struct outbuf *buf, uint8 val);
void outbuf_write_u16(struct outbuf *buf, uint16 val);
void outbuf_write_u32(struct outbuf *buf, uint32 val);
void outbuf_write_str(struct outbuf *buf, const char *s);
void outbuf_write_lstr(struct outbuf *buf, const char *s, int len);

#endif //KAPLAR_OUTBUF_H_
