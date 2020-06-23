#include "iocp.h"

#ifdef PLATFORM_WINDOWS

#include "../buffer_util.h"
#include "../config.h"
#include "../log.h"
#include "../thread.h"

// NOTE1:	login messages are always 149 bytes
// NOTE2:	game messages are usually small (less than 16 bytes w/o strings)
// NOTE3:	tibia protocols use little endian encoding so we need
//		to keep that in mind when decoding the message length.

// NOTE4:	having the input buffer size of 1024 bytes should be
//		enough for our use case as having an input message of
//		more than 1KB would require the client to send us a large
//		string which cannot be done (need to verify if there is
//		a limit to player_say, player_text_window or player_house_window)

/* Connection Structure */

// connection settings
#define MAX_CONNECTIONS			8192
#if MAX_CONNECTIONS > UINT16_MAX
#	error "Max number of concurrent connections should not exceed UINT16_MAX."
#endif
#define CONN_INPUT_BUFFER_SIZE		1024
#define CONN_OP_RETRIES_BEFORE_CLOSING	5

// connection flags
#define CONN_INUSE			0x01
#define CONN_CLOSING			0x04
#define CONN_FIRST_MSG			0x08
#define CONN_OUTPUT_IN_PROGRESS		0x10

struct conn_ctl{
	uint32 uid;
	uint32 flags;

	SOCKET s;
	int32 rdwr_count;
	int32 pending_work;
	struct protocol *proto;
	void *proto_handle;
};

struct conn_input_ctl{
	struct async_ov read_ov;
	uint32 input_len;
	uint8 input_buffer[CONN_INPUT_BUFFER_SIZE];
};

struct conn_output_ctl{
	struct async_ov write_ov;
	uint32 datalen;
	uint8 *data;
};

/* Connection List */
static struct conn_ctl ctl[MAX_CONNECTIONS];
static struct conn_input_ctl input_ctl[MAX_CONNECTIONS];
static struct conn_output_ctl output_ctl[MAX_CONNECTIONS];
// the connection address will be only used when we add IP
// bans and the sort but for now lets just leave it away
//static struct sockaddr_in conn_addr[MAX_CONNECTIONS];

static uint16 freelist = UINT16_MAX;
static uint16 next_unused_slot = 0;


// NOTE: (FOR ALL CONNECTIONS SLOTS UNDER `next_unused_slot`)
//	When a connection is INUSE, it's `uid` field has the
// current slot counter and the current slot position. When a
// connection is NOT INUSE, it's `uid` field has the previous
// slot counter and the slot position of the next item on the
// freelist.

static struct conn_ctl *internal_alloc(void){
	uint16 slot;
	struct conn_ctl *c;

	if(freelist != UINT16_MAX){
		slot = freelist;
		c = &ctl[slot];
		freelist = (uint16)(c->uid & 0xFFFF);
	}else{
		if(next_unused_slot >= MAX_CONNECTIONS)
			return NULL;
		slot = next_unused_slot++;
		c = &ctl[slot];
	}
	c->uid = ((c->uid + 0x00010000) & 0xFFFF0000) | (uint32)slot;
	return c;
}
static void internal_free(struct conn_ctl *c){
	uint16 slot = c->uid & 0xFFFF;
	DEBUG_ASSERT(c == &ctl[slot]);
	DEBUG_ASSERT(c->flags & CONN_INUSE);
	c->uid = (c->uid & 0xFFFF0000) | freelist;
	freelist = slot;
	c->flags ^= CONN_INUSE;
}

#define CONN_CTL(uid)		(&ctl[uid & 0xFFFF])
#define CONN_INPUT_CTL(c)	(&input_ctl[(c)->uid & 0xFFFF])
#define CONN_OUTPUT_CTL(c)	(&output_ctl[(c)->uid & 0xFFFF])

/* STATIC FWD DECL */
static void internal_dispatch_on_close(struct conn_ctl *c);
static protocol_status_t internal_dispatch_on_connect(struct conn_ctl *c);
static protocol_status_t internal_dispatch_on_recv_message(
		struct conn_ctl *c, uint8 *data, uint32 datalen);
static protocol_status_t internal_dispatch_on_write(struct conn_ctl *c);
static void internal_on_read_length(void *data, DWORD err, DWORD transferred);
static void internal_on_read_body(void *data, DWORD err, DWORD transferred);
static void internal_on_write(void *data, DWORD err, DWORD transferred);
static bool internal_start_async_read(struct conn_ctl *c, struct conn_input_ctl *in,
		void (*callback)(void*, DWORD, DWORD));
static void internal_async_read(struct conn_ctl *c, struct conn_input_ctl *in,
		void (*callback)(void*, DWORD, DWORD));
static bool internal_start_async_write(struct conn_ctl *c, struct conn_output_ctl *out);
static void internal_async_write(struct conn_ctl *c, struct conn_output_ctl *out);
static void internal_close_operation(struct conn_ctl *c);
static void internal_start_close_operation(struct conn_ctl *c, bool abort);
static void internal_close(struct conn_ctl *c);
static void internal_abort(struct conn_ctl *c);

/* IMPL START */
static INLINE
void internal_dispatch_on_close(struct conn_ctl *c){
	if(c->proto != NULL){
		c->proto->on_close(c->uid, c->proto_handle);
		c->proto->destroy_handle(c->uid, c->proto_handle);
		c->proto_handle = NULL;
		c->proto = NULL;
	}
}

static INLINE protocol_status_t
internal_dispatch_on_connect(struct conn_ctl *c){
	// the connection was just made so:
	//	c->proto == NULL
	// and	c->proto_handle == service
	DEBUG_ASSERT(c->proto == NULL);
	DEBUG_ASSERT(c->proto_handle != NULL);
	struct service *svc = c->proto_handle;
	if(!service_sends_first(svc))
		return PROTO_OK;
	c->proto = service_first_protocol(svc);
	DEBUG_ASSERT(c->proto != NULL);
	if(!c->proto->create_handle(c->uid, &c->proto_handle))
		return PROTO_ABORT;
	return c->proto->on_connect(c->uid, c->proto_handle);
}

static INLINE protocol_status_t
internal_dispatch_on_write(struct conn_ctl *c){
	DEBUG_ASSERT(c->proto != NULL);
	return c->proto->on_write(c->uid, c->proto_handle);
}

static INLINE protocol_status_t
internal_dispatch_on_recv_message(struct conn_ctl *c, uint8 *data, uint32 datalen){
	// select protocol if this is the first message
	if(c->proto == NULL){
		// if	c->proto == NULL
		// then	c->proto_handle == service
		DEBUG_ASSERT(c->proto_handle != NULL);
		c->proto = service_select_protocol(c->proto_handle, data, datalen);
		if(!c->proto || !c->proto->create_handle(c->uid, &c->proto_handle))
			return PROTO_ABORT;
	}
	// dispatch message
	if(!(c->flags & CONN_FIRST_MSG)){
		c->flags |= CONN_FIRST_MSG;
		return c->proto->on_recv_first_message(
			c->uid, c->proto_handle, data, datalen);
	}
	return c->proto->on_recv_message(
		c->uid, c->proto_handle, data, datalen);
}

static void internal_on_read_length(void *data, DWORD err, DWORD transferred){
	struct conn_ctl *c = data;
	struct conn_input_ctl *in = CONN_INPUT_CTL(c);
	uint16 body_length;

	// decrease pending work
	c->pending_work -= 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			internal_close_operation(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		internal_async_read(c, in, internal_on_read_length);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		internal_abort(c);
		return;
	}

	// this should not happen
	if(transferred != 2){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		internal_abort(c);
		return;
	}

	// decode body length
	body_length = decode_u16_le(in->input_buffer);

	// assert that body_length isn't zero or overflows the input buffer
	if(body_length > CONN_INPUT_BUFFER_SIZE || body_length == 0){
		internal_abort(c);
		return;
	}

	// chain body read
	in->input_len = body_length;
	internal_async_read(c, in, internal_on_read_body);
}

static void internal_on_read_body(void *data, DWORD err, DWORD transferred){
	struct conn_ctl *c = data;
	struct conn_input_ctl *in = CONN_INPUT_CTL(c);

	// decrease pending work
	c->pending_work -= 1;
	// increase read count when we receive the
	// message body but not otherwise
	c->rdwr_count += 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			internal_close_operation(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		internal_async_read(c, in, internal_on_read_body);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		internal_abort(c);
		return;
	}

	// this should not happen
	if(in->input_len != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		internal_abort(c);
		return;
	}

	// dispatch message to protocol
	switch(internal_dispatch_on_recv_message(c,
		in->input_buffer, in->input_len)){
	case PROTO_OK: break;
	case PROTO_STOP_READING: return;
	case PROTO_CLOSE:
		internal_close(c);
		return;
	case PROTO_ABORT:
	default:
		internal_abort(c);
		return;
	}

	// chain next message read
	in->input_len = 2;
	internal_async_read(c, in, internal_on_read_length);
}

static void internal_on_write(void *data, DWORD err, DWORD transferred){
	struct conn_ctl *c = data;
	struct conn_output_ctl *out = CONN_OUTPUT_CTL(c);

	// decrease pending work
	c->pending_work -= 1;
	// increase write count
	c->rdwr_count += 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			internal_close_operation(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		internal_async_write(c, out);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		internal_abort(c);
		return;
	}

	// this should not happen
	if(out->datalen != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: write operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		internal_abort(c);
		return;
	}

	// output operation has completed
	c->flags &= ~CONN_OUTPUT_IN_PROGRESS;

	// notify the protocol that the write succeded
	switch(internal_dispatch_on_write(c)){
	case PROTO_OK: break;
	case PROTO_STOP_READING:
		UNREACHABLE();
		break;
	case PROTO_CLOSE:
		internal_close(c);
		return;
	case PROTO_ABORT:
	default:
		internal_abort(c);
		return;
	}
}

static bool internal_start_async_read(struct conn_ctl *c, struct conn_input_ctl *in,
		void (*callback)(void*, DWORD, DWORD)){
	WSABUF wsabuf;
	DWORD error, flags;
	int ret;
	// prepare buffer
	wsabuf.len = in->input_len;
	wsabuf.buf = in->input_buffer;
	// prepare overlapped
	memset(&in->read_ov.ov, 0, sizeof(OVERLAPPED));
	in->read_ov.s = c->s;
	in->read_ov.complete = callback;
	in->read_ov.data = c;
	// dispatch to OS
	// MSG_WAITALL ensures that the amount requested is
	// received before queuing a completion socket
	flags = MSG_WAITALL;
	ret = WSARecv(c->s, &wsabuf, 1, NULL,
		&flags, &in->read_ov.ov, NULL);
	if(ret == SOCKET_ERROR){
		error = WSAGetLastError();
		if(error != WSA_IO_PENDING){
			LOG_ERROR("connection_start_async_read:"
				" WSARecv failed (error = %d)", error);
			return false;
		}
	}
	c->pending_work += 1;
	return true;
}

static void internal_async_read(struct conn_ctl *c, struct conn_input_ctl *in,
		void (*callback)(void*, DWORD, DWORD)){
	DEBUG_ASSERT(c != NULL);
	DEBUG_ASSERT(in != NULL && in->input_len > 0);
	DEBUG_ASSERT(callback != NULL);
	for(int i = 0; i < CONN_OP_RETRIES_BEFORE_CLOSING; i += 1){
		if(internal_start_async_read(c, in, callback))
			return;
		DEBUG_LOG("connection_async_read: failed to"
			" start read operation (try %d)", i);
	}
	DEBUG_LOG("connection_async_read: failed to start"
		" read operation after %d tries...",
		CONN_OP_RETRIES_BEFORE_CLOSING);
	DEBUG_LOG("connection_async_read: aborting connection...");
	internal_abort(c);
}

static bool internal_start_async_write(struct conn_ctl *c, struct conn_output_ctl *out){
	WSABUF wsabuf;
	DWORD error;
	int ret;
	// prepare buffer
	wsabuf.len = out->datalen;
	wsabuf.buf = out->data;
	// prepare overlapped
	memset(&out->write_ov.ov, 0, sizeof(OVERLAPPED));
	out->write_ov.s = c->s;
	out->write_ov.complete = internal_on_write;
	out->write_ov.data = c;
	// dispatch to OS
	ret = WSASend(c->s, &wsabuf, 1, NULL,
		0, &out->write_ov.ov, NULL);
	if(ret == SOCKET_ERROR){
		error = WSAGetLastError();
		if(error != WSA_IO_PENDING){
			LOG_ERROR("connection_start_async_write:"
				" WSASend failed (error = %d)", error);
			return false;
		}
	}
	c->flags |= CONN_OUTPUT_IN_PROGRESS;
	c->pending_work += 1;
	return true;
}

static void internal_async_write(struct conn_ctl *c, struct conn_output_ctl *out){
	DEBUG_ASSERT(c != NULL);
	DEBUG_ASSERT(out != NULL);
	for(int i = 0; i < CONN_OP_RETRIES_BEFORE_CLOSING; i += 1){
		if(internal_start_async_write(c, out))
			return;
		DEBUG_LOG("connection_async_write: failed to"
			" start write operation (try %d)", i);
	}
	DEBUG_LOG("connection_async_write: failed to start"
		" write operation after %d tries...",
		CONN_OP_RETRIES_BEFORE_CLOSING);
	DEBUG_LOG("connection_async_write: aborting connection...");
	internal_abort(c);
}

static void internal_close_operation(struct conn_ctl *c){
	DEBUG_ASSERT(c != NULL);
	DEBUG_ASSERT(c->s != INVALID_SOCKET);
	// dispatch protocol close
	internal_dispatch_on_close(c);
	// close socket
	closesocket(c->s);
	// release connection
	internal_free(c);
}

static void internal_start_close_operation(struct conn_ctl *c, bool abort){
	DEBUG_ASSERT(c != NULL);
	if(c->pending_work == 0){
		internal_close_operation(c);
	}else{
		c->flags |= CONN_CLOSING;
		if(abort){
			// cancel all socket operations
			CancelIoEx((HANDLE)c->s, NULL);
		}else{
			// cancel only the read operation and let the
			// write operation complete
			struct conn_input_ctl *in = CONN_INPUT_CTL(c);
			CancelIoEx((HANDLE)c->s, &in->read_ov.ov);
		}
	}
}

static INLINE void internal_close(struct conn_ctl *c){
	internal_start_close_operation(c, false);
}

static INLINE void internal_abort(struct conn_ctl *c){
	internal_start_close_operation(c, true);
}

bool connmgr_init(void){
	// initializing the connection uids is unnecessary
	return true;
}

void connmgr_shutdown(void){
	struct conn_ctl *c = &ctl[0];
	struct conn_ctl *end = &ctl[next_unused_slot];
	while(c < end){
		if(c->flags & CONN_INUSE)
			internal_close_operation(c);
		c += 1;
	}
}

void connmgr_timeout_check(void){
	struct conn_ctl *c = &ctl[0];
	struct conn_ctl *end = &ctl[next_unused_slot];
	while(c < end){
		if(c->flags & CONN_INUSE){
			if(c->rdwr_count == 0)
				internal_abort(c);
			else
				c->rdwr_count = 0;
		}
		c += 1;
	}
}

void connmgr_start_connection(SOCKET s,
		struct sockaddr_in *addr,
		struct service *svc){
	DEBUG_ASSERT(s != INVALID_SOCKET);
	DEBUG_ASSERT(svc != NULL);
	struct conn_ctl *c = internal_alloc();
	struct conn_input_ctl *in = CONN_INPUT_CTL(c);

	// connection control
	DEBUG_ASSERT(!(c->flags & CONN_INUSE));
	c->flags = CONN_INUSE;
	c->s = s;
	c->rdwr_count = 1; // don't timeout a new connection too soon
	c->pending_work = 0;
	c->proto = NULL;
	c->proto_handle = svc;

	// this only works for sends_first protocols, and will
	// create the protocol and notify it of the connection
	switch(internal_dispatch_on_connect(c)){
	case PROTO_OK: break;
	case PROTO_STOP_READING:
		UNREACHABLE();
		return;
	case PROTO_CLOSE:
		internal_close(c);
		return;
	case PROTO_ABORT:
	default:
		internal_abort(c);
		return;
	}

	// start receiving messages from the connection
	in->input_len = 2;
	if(!internal_start_async_read(c, in, internal_on_read_length))
		// if the first attempt to start a connection read fails,
		// just abort the connection
		internal_abort(c);
}

void connection_close(uint32 uid){
	struct conn_ctl *c = CONN_CTL(uid);
	if(c->uid == uid)
		internal_close(c);
}

void connection_abort(uint32 uid){
	struct conn_ctl *c = CONN_CTL(uid);
	if(c->uid == uid)
		internal_abort(c);
}

bool connection_send(uint32 uid, uint8 *data, uint32 datalen){
	struct conn_ctl *c = CONN_CTL(uid);
	struct conn_output_ctl *out;
	if(c->uid != uid){
		DEBUG_LOG("connection_send: using invalid connection uid (%lu)", uid);
		return false;
	}
	if(c->flags & CONN_OUTPUT_IN_PROGRESS){
		DEBUG_LOG("connection_send: trying to send message while there"
			" is already an ongoing output operation");
		return false;
	}

	out = CONN_OUTPUT_CTL(c);
	out->datalen = datalen;
	out->data = data;
	return internal_start_async_write(c, out);
}

#endif //PLATFORM_WINDOWS
