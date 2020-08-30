#include "outbuf.h"
#include "buffer_util.h"
#include "thread.h"

/* outbuf list control */
static mutex_t outbuf_mtx;
static int32 outbuf_list_size = 0;
static struct outbuf *outbuf_head = NULL;

bool outbuf_init(void){
	mutex_init(&outbuf_mtx);
	return true;
}

void outbuf_shutdown(void){
	mutex_destroy(&outbuf_mtx);
}

struct outbuf *server_outbuf_acquire(void){
	struct outbuf *buf = NULL;
	mutex_lock(&outbuf_mtx);
	if(outbuf_head != NULL){
		DEBUG_ASSERT(outbuf_list_size > 0);
		outbuf_list_size -= 1;
		buf = outbuf_head;
		outbuf_head = buf->next;
	}
	mutex_unlock(&outbuf_mtx);
	if(buf == NULL)
		buf = kpl_malloc(sizeof(struct outbuf));
	return buf;
}

void server_outbuf_release(struct outbuf *buf){
	mutex_lock(&outbuf_mtx);
	if(outbuf_list_size < MAX_IDLE_OUTBUFS){
		outbuf_list_size += 1;
		buf->next = outbuf_head;
		outbuf_head = buf;
		buf = NULL;
	}
	mutex_unlock(&outbuf_mtx);
	if(buf != NULL)
		kpl_free(buf);
}

/* write functions */

#define DEBUG_CHECK_SIZE(buf, size) \
	DEBUG_ASSERT(((buf)->ptr - (buf)->base) <= (MAX_OUTBUF_LEN - (size)))

void outbuf_write_byte(struct outbuf *buf, uint8 val){
	DEBUG_CHECK_SIZE(buf, 1);
	encode_u8(buf->ptr, val);
	buf->ptr += 1;
}

void outbuf_write_u16(struct outbuf *buf, uint16 val){
	DEBUG_CHECK_SIZE(buf, 2);
	encode_u16_le(buf->ptr, val);
	buf->ptr += 2;
}

void outbuf_write_u32(struct outbuf *buf, uint32 val){
	DEBUG_CHECK_SIZE(buf, 4);
	encode_u32_le(buf->ptr, val);
	buf->ptr += 4;
}

void outbuf_write_str(struct outbuf *buf, const char *s){
	outbuf_write_lstr(buf, s, (int)strlen(s));
}

void outbuf_write_lstr(struct outbuf *buf, const char *s, int len){
	int total = len + 2;
	DEBUG_CHECK_SIZE(buf, total);
	encode_u16_le(buf->ptr, (uint16)len);
	if(len > 0) memcpy(buf->ptr + 2, s, len);
	buf->ptr += total;
}
