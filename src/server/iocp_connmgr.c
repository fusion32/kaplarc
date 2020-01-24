/* NOTE1: the usage of double buffering is reasonable in here
 * because the Tibia protocol is fairly simple and there is
 * no message size greater than a couple KBs and no file transfers
 */

/* NOTE2: output double buffering is quite simple to implement
 * but input double buffering should be done on a protocol level
 * by copying the recently received message to another buffer while
 * leaving the connection input buffer free
 */

#include "iocp.h"

#ifdef PLATFORM_WINDOWS

#include "../buffer_util.h"
#include "../config.h"
#include "../slab_cache.h"
#include "../thread.h"

/* Connection Structure */
// connection settings
#define CONN_INPUT_SIZE 32768
#define CONN_OUTPUT_SIZE 32768
#define CONN_TRIES_BEFORE_CLOSING 5
// connection command flags
#define CONN_CLOSING 0x01
#define CONN_SWAP_OUTPUT 0x02
// connection status flags
#define CONN_FIRST_MSG 0x100
#define CONN_WAITING_OUTPUT_SWAP 0x200
struct connection{
	SOCKET s;
	uint32 flags;
	int pending_work;
	//uint32 rdwr_count;
	struct sockaddr_in addr;
	struct service *svc;
	struct protocol *proto;
	void *proto_handle;
	// overlapped structs
	struct async_ov read_ov;
	struct async_ov write_ov;
	// input buffer
	ULONG input_body_length;
	uint8 *input_ptr;
	uint8 input_buffer[CONN_INPUT_SIZE];
	// output double buffer
	int output_idx;
	uint32 output_length[2];
	uint8 output_buffer[2][CONN_OUTPUT_SIZE];
	// connection list
	struct connection *next;
	struct connection *prev;
};

/* Connection Manager Data */
static struct connection *conn_head = NULL;
static struct slab_cache *connections = NULL;

/* STATIC FWD DECL */
static void connection_on_connect(struct connection *c);
static void connection_on_recv_message(struct connection *c);
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
static void connection_start_closing(struct connection *c);

/* IMPL START */
//@TODO
static void connection_on_connect(struct connection *c){}
static void connection_on_recv_message(struct connection *c){}

static void connection_on_read_length(void *data, DWORD err, DWORD transferred){
	struct connection *c = data;

	// decrease pending work
	c->pending_work -= 1;

	// handle connection closing
	if(c->flags & CONN_CLOSING){
		if(c->pending_work == 0)
			__connection_close(c);
		return;
	}

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_read(c, 2,
			connection_on_read_length);
		return;
	}

	// this should not happen
	if(transferred != 2){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_start_closing(c);
		return;
	}

	// decode body length and advance input pointer
	c->input_body_length = decode_u16_be(c->input_ptr);
	c->input_ptr += 2;

	// chain body read
	connection_async_read(c, c->input_body_length,
		connection_on_read_body);
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

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_read(c, c->input_body_length,
			connection_on_read_body);
		return;
	}

	// this should not happen
	if(c->input_body_length != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: read operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_start_closing(c);
		return;
	}

	// dispatch message to protocol
	connection_on_recv_message(c);

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

	// handle connection errors
	if(err != NOERROR){
		// retry operation
		connection_async_write(c);
		return;
	}

	// this should not happen
	if(c->output_length[c->output_idx] != transferred){
		DEBUG_ASSERT(0 &&
			"WS ERROR: write operation completed with"
			" a transferred amount diferent from the"
			" requested amount");
		connection_start_closing(c);
		return;
	}

	// chain next write if there is a swap output command,
	// else signal that we are done writting
	if(c->flags & CONN_SWAP_OUTPUT){
		c->output_idx = 1 - c->output_idx;
		c->flags &= ~CONN_SWAP_OUTPUT;
		connection_async_write(c);
	}else{
		c->flags |= CONN_WAITING_OUTPUT_SWAP;
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
	flags = 0;
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
	int output_idx;
	int ret;
	// prepare buffer
	output_idx = c->output_idx;
	wsabuf.len = c->output_length[output_idx];
	wsabuf.buf = c->output_buffer[output_idx];
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
	DEBUG_LOG("connection_async_read: closing connection...");
	connection_start_closing(c);
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
	DEBUG_LOG("connection_async_write: closing connection...");
	connection_start_closing(c);
}

static void __connection_close(struct connection *c){
	// remove from connection list
	if(c->prev) c->prev->next = c->next;
	if(c->next) c->next->prev = c->prev;
	// release connection
	closesocket(c->s);
	slab_cache_free(connections, c);
}

static void connection_start_closing(struct connection *c){
	DEBUG_ASSERT(c != NULL);
	if(c->pending_work == 0){
		__connection_close(c);
	}else{
		c->flags |= CONN_CLOSING;
		CancelIoEx((HANDLE)c->s, NULL);
	}
}

static void connection_swap_output(struct connection *c){
	if(c->flags & CONN_WAITING_OUTPUT_SWAP){
		c->output_idx = 1 - c->output_idx;
		c->flags &= ~CONN_WAITING_OUTPUT_SWAP;
		connection_async_write(c);
	}else{
		c->flags |= CONN_SWAP_OUTPUT;
	}
}

/*
uint8 *connection_get_output_buffer(struct connection *c, size_t *capacity){
	return NULL;
}
*/

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

void connmgr_start_shutdown(void){
	struct connection *c;
	for(c = conn_head; c != NULL; c = c->next)
		connection_start_closing(c);
}

void connmgr_shutdown(void){
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
	//c->rdwr_count = 0;
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
	c->output_idx = 0;
	c->output_length[0] = 0;
	c->output_length[1] = 0;
	// insert into connection list
	c->prev = NULL;
	c->next = conn_head;
	conn_head->prev = c;
	conn_head = c;

	// this only works for sends_first protocols, and will
	// create the protocol and notify it of the connection
	connection_on_connect(c);

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

#endif //PLATFORM_WINDOWS
