#ifndef KAPLAR_OUTBUF_H_
#define KAPLAR_OUTBUF_H_ 1

#include "def.h"

// outbuf interface
//	NOTES:
//	- The outbuf interface is a simple linked list of
//	available free output buffers. If there is none
//	available, one is allocated and returned.
//	- There is no internal tracking of the buffers so
//	they must be manually managed.
//	- acquire and release are thread safe.

#define MAX_IDLE_OUTBUFS 1024
#define MAX_OUTBUF_LEN (16384 - 2*sizeof(void*))
struct outbuf{
	struct outbuf *next;
	uint32 datalen;
	uint8 data[MAX_OUTBUF_LEN];
};
bool outbuf_init(void);
void outbuf_shutdown(void);
struct outbuf *outbuf_acquire(void);
void outbuf_release(struct outbuf *buf);

#endif // KAPLAR_OUTBUF_H_
