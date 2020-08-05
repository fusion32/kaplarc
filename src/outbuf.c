#include "outbuf.h"
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
