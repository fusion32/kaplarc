#include "iocp.h"

#ifdef PLATFORM_WINDOWS

#include "../buffer_util.h"
#include "../config.h"
#include "../slab_cache.h"
#include "../thread.h"

/* Connection Structure */
// connection settings
#define CONN_INPUT_SIZE 32768
#define CONN_MAX_BODY_SIZE (CONN_INPUT_SIZE - 2)
#define CONN_TRIES_BEFORE_CLOSING 5
// connection flags
#define CONN_CLOSING		0x01
#define CONN_FIRST_MSG		0x02
#define CONN_OUTPUT_IN_PROGRESS	0x04
struct connection{
	SOCKET s;
	uint32 flags;
	int pending_work;
	struct sockaddr_in addr;
	struct service *svc;
	struct protocol *proto;
	void *proto_handle;
	// input operation
	struct async_ov read_ov;
	ULONG input_body_length;
	uint8 *input_ptr;
	uint8 input_buffer[CONN_INPUT_SIZE];
	// output operation
	struct async_ov write_ov;
	uint32 output_length;
	uint8 *output_data;
	// connection list
	struct connection *next;
	struct connection *prev;
};

/* Connection Manager Data */
static struct connection *conn_head = NULL;
static struct slab_cache *connections = NULL;

/* STATIC FWD DECL */
static void connection_dispatch_on_close(struct connection *c);
static protocol_status_t connection_dispatch_on_connect(struct connection *c);
static protocol_status_t connection_dispatch_on_recv_message(struct connection *c);
static protocol_status_t connection_dispatch_on_write(struct connection *c);
static void connection_on_read_length(void *data, DWORD err, DWORD transferred);
static void connection_on_read_body(void *data, DWORD err, DWORD transferred);
static void connection_on_write(void *data, DWORD err, DWORD transferred);
static bool connection_start_async_read(struct connection *c, ULONG len,
		void (*callback)(void*, DWORD, DWORD));
static bool connection_start_async_write(struct connection *c);
static void connection_async_read(struct connection *c, ULONG len,
		void (*callback)(void*, DWORD, DWORD));
static void connection_async_write(struct connection *c);
static void __connection_close(struct connection *c);
static void connection_start_closing(struct connection *c, bool abort);

/* IMPL START */
static INLINE
void connection_dispatch_on_close(struct connection *c){
	if(c->proto != NULL){
		c->proto->on_close(c, c->proto_handle);
		c->proto->destroy_handle(c, c->proto_handle);
		c->proto_handle = NULL;
		c->proto = NULL;
	}
}

static INLINE protocol_status_t
connection_dispatch_on_connect(struct connection *c){
	if(!service_sends_first(c->svc))
		return PROTO_OK;
	c->proto = service_first_protocol(c->svc);
	DEBUG_ASSERT(c->proto != NULL);
	if(!c->proto->create_handle(c, &c->proto_handle))
		return PROTO_ABORT;
	return c->proto->on_connect(c, c->proto_handle);
}

static INLINE protocol_status_t
connection_dispatch_on_write(struct connection *c){
	DEBUG_ASSERT(c->proto != NULL);
	return c->proto->on_write(c, c->proto_handle);
}

static INLINE protocol_status_t
connection_dispatch_on_recv_message(struct connection *c){
	uint8 *data = c->input_ptr;
	uint32 datalen = c->input_body_length;

	// select protocol if this is the first message
	if(c->proto == NULL){
		c->proto = service_select_protocol(c->svc, data, datalen);
		if(!c->proto || !c->proto->create_handle(c, &c->proto_handle))
			return PROTO_ABORT;
	}

	if(!(c->flags & CONN_FIRST_MSG)){
		c->flags |= CONN_FIRST_MSG;
		return c->proto->on_recv_first_message(c,
			c->proto_handle, data, datalen);
	}
	return c->proto->on_recv_message(c,
		c->proto_handle, data, datalen);
}

static void connection_on_read_length(void *data, DWORD err, DWORD transferred){
	struct connection *c = data;
	uint16 body_length;

	// decrease pending work
	c->pending_work -= 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			__connection_close(c);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		connection_abort(c);
		return;
	}

	// this should not happen
	if(transferred != 2){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_abort(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_read(c, 2, connection_on_read_length);
		return;
	}

	// decode body length and advance input pointer
	// @NOTE: tibia protocols use little endian encoding
	body_length = decode_u16_le(c->input_ptr);
	c->input_ptr += 2;

	// assert that body_length isn't zero or
	// overflows the input buffer
	if(body_length > CONN_MAX_BODY_SIZE || body_length == 0){
		connection_abort(c);
		return;
	}

	// chain body read
	c->input_body_length = body_length;
	connection_async_read(c, body_length, connection_on_read_body);
}

static void connection_on_read_body(void *data, DWORD err, DWORD transferred){
	struct connection *c = data;

	// decrease pending work
	c->pending_work -= 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			__connection_close(c);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		connection_abort(c);
		return;
	}

	// this should not happen
	if(c->input_body_length != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_abort(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_read(c, c->input_body_length,
			connection_on_read_body);
		return;
	}

	// dispatch message to protocol
	switch(connection_dispatch_on_recv_message(c)){
	case PROTO_OK: break;
	case PROTO_CLOSE:
		connection_close(c);
		return;
	case PROTO_ABORT:
	default:
		connection_abort(c);
		return;
	}

	// reset input pointer
	c->input_ptr = c->input_buffer;
	// chain next message read
	connection_async_read(c, 2, connection_on_read_length);
}

static void connection_on_write(void *data, DWORD err, DWORD transferred){
	struct connection *c = data;

	// decrease pending work
	c->pending_work -= 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			__connection_close(c);
		return;
	}

	// connection closed by peer
	if(transferred == 0){
		connection_abort(c);
		return;
	}

	// this should not happen
	if(c->output_length != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: write operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_abort(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_write(c);
		return;
	}

	// output operation has completed
	c->flags &= ~CONN_OUTPUT_IN_PROGRESS;

	// notify the protocol that the write succeded
	switch(connection_dispatch_on_write(c)){
	case PROTO_OK: break;
	case PROTO_CLOSE:
		connection_close(c);
		return;
	case PROTO_ABORT:
	default:
		connection_abort(c);
		return;
	}
}

static bool connection_start_async_read(struct connection *c, ULONG len,
		void (*callback)(void*, DWORD, DWORD)){
	WSABUF wsabuf;
	DWORD error, flags;
	int ret;
	// prepare buffer
	wsabuf.len = len;
	wsabuf.buf = c->input_ptr;
	// prepare overlapped
	memset(&c->read_ov.ov, 0, sizeof(OVERLAPPED));
	c->read_ov.s = c->s;
	c->read_ov.complete = callback;
	c->read_ov.data = c;
	// dispatch to OS
	// MSG_WAITALL ensures that the amount requested is
	// received before queuing a completion socket
	flags = MSG_WAITALL;
	ret = WSARecv(c->s, &wsabuf, 1, NULL,
		&flags, &c->read_ov.ov, NULL);
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

static bool connection_start_async_write(struct connection *c){
	WSABUF wsabuf;
	DWORD error;
	int ret;
	// prepare buffer
	wsabuf.len = c->output_length;
	wsabuf.buf = c->output_data;
	// prepare overlapped
	memset(&c->write_ov.ov, 0, sizeof(OVERLAPPED));
	c->write_ov.s = c->s;
	c->write_ov.complete = connection_on_write;
	c->write_ov.data = c;
	// dispatch to OS
	ret = WSASend(c->s, &wsabuf, 1, NULL,
		0, &c->write_ov.ov, NULL);
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

static void connection_async_read(struct connection *c, ULONG len,
		void (*callback)(void*, DWORD, DWORD)){
	DEBUG_ASSERT(c != NULL);
	DEBUG_ASSERT(len > 0);
	DEBUG_ASSERT(callback != NULL);
	for(int i = 0; i < CONN_TRIES_BEFORE_CLOSING; i += 1){
		if(connection_start_async_read(c, len, callback))
			return;
		DEBUG_LOG("connection_async_read: failed to"
			" start read operation (try %d)", i);
	}
	DEBUG_LOG("connection_async_read: failed to start"
		" read operation after %d tries...",
		CONN_TRIES_BEFORE_CLOSING);
	DEBUG_LOG("connection_async_read: aborting connection...");
	connection_abort(c);
}

static void connection_async_write(struct connection *c){
	DEBUG_ASSERT(c != NULL);
	for(int i = 0; i < CONN_TRIES_BEFORE_CLOSING; i += 1){
		if(connection_start_async_write(c))
			return;
		DEBUG_LOG("connection_async_write: failed to"
			" start write operation (try %d)", i);
	}
	DEBUG_LOG("connection_async_write: failed to start"
		" write operation after %d tries...",
		CONN_TRIES_BEFORE_CLOSING);
	DEBUG_LOG("connection_async_write: aborting connection...");
	connection_abort(c);
}

static void __connection_close(struct connection *c){
	DEBUG_ASSERT(c != NULL);
	DEBUG_ASSERT(c->s != INVALID_SOCKET);
	// dispatch protocol close
	connection_dispatch_on_close(c);
	// remove from connection list
	if(c->prev) c->prev->next = c->next;
	if(c->next) c->next->prev = c->prev;
	// release connection
	closesocket(c->s);
	slab_cache_free(connections, c);
}

static void connection_start_closing(struct connection *c, bool abort){
	DEBUG_ASSERT(c != NULL);
	if(c->pending_work == 0){
		__connection_close(c);
	}else{
		c->flags |= CONN_CLOSING;
		if(abort){
			// cancel all socket operations
			CancelIoEx((HANDLE)c->s, NULL);
		}else{
			// cancel only read operation and let the
			// write operation complete
			CancelIoEx((HANDLE)c->s, &c->read_ov.ov);
		}
	}
}

bool connmgr_init(void){
	connections = slab_cache_create(
		config_geti("conn_slots_per_slab"),
		sizeof(struct connection));
	if(!connections){
		LOG_ERROR("connmgr_init: failed to create"
			" connection slab cache");
		return false;
	}
	return true;
}

void connmgr_shutdown(void){
	struct connection *c;
	// close all connections
	for(c = conn_head; c != NULL; c = c->next)
		__connection_close(c);
	// release connections memory
	slab_cache_destroy(connections);
}

void connmgr_start_connection(SOCKET s,
		struct sockaddr_in *addr,
		struct service *svc){
	DEBUG_ASSERT(s != INVALID_SOCKET);
	DEBUG_ASSERT(svc != NULL);
	struct connection *c = slab_cache_alloc(connections);
	DEBUG_ASSERT(c != NULL);

	// init connection
	c->s = s;
	c->flags = 0;
	c->pending_work = 0;
	if(addr != NULL)
		memcpy(&c->addr, addr, sizeof(struct sockaddr_in));
	else
		memset(&c->addr, 0, sizeof(struct sockaddr_in));
	c->svc = svc;
	c->proto = NULL;
	c->proto_handle = NULL;
	// init buffers
	c->input_body_length = 0;
	c->input_ptr = c->input_buffer;
	c->output_length = 0;
	c->output_data = NULL;
	// insert into connection list
	c->prev = NULL;
	c->next = conn_head;
	if(conn_head)
		conn_head->prev = c;
	conn_head = c;

	// this only works for sends_first protocols, and will
	// create the protocol and notify it of the connection
	switch(connection_dispatch_on_connect(c)){
	case PROTO_OK: break;
	case PROTO_CLOSE:
		connection_close(c);
		return;
	case PROTO_ABORT:
	default:
		connection_abort(c);
		return;
	}

	// if the first reading atempt fails, abort connection
	if(!connection_start_async_read(c, 2,
			connection_on_read_length)){
		// remove from connection list
		conn_head = conn_head->next;
		conn_head->prev = NULL;
		// free connection
		closesocket(s);
		slab_cache_free(connections, c);
	}
}

INLINE
void connection_close(struct connection *c){
	connection_start_closing(c, false);
}

INLINE
void connection_abort(struct connection *c){
	connection_start_closing(c, true);
}

bool connection_send(struct connection *c, uint8 *data, uint32 datalen){
	if(c->flags & CONN_OUTPUT_IN_PROGRESS)
		return false;
	c->output_length = datalen;
	c->output_data = data;
	return connection_start_async_write(c);
}

#endif //PLATFORM_WINDOWS
