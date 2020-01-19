
#if 0
/*
case WSAENETRESET:
case WSAECONNABORTED:
case WSAECONNRESET:
case WSAETIMEDOUT:
case WSAEDISCON: // graceful shutdown
*/

#include "iocp.h"
#include "../config.h"
#include "../def.h"
#include "../slab_cache.h"
#include "../thread.h"


struct double_buffer{
	uint32 length[2];
	uint8 *buffers[2];
};

#define CONNECTION_NEW		0x00
#define CONNECTION_READY	0x01
#define CONNECTION_CLOSING	0x02
#define CONNECTION_FIRST_MSG	0x04
#define CONNECTION_RD_DONE	0x08
#define CONNECTION_WR_DONE	0x10
struct connection{
	SOCKET s;
	uint32 flags;
	uint32 ref_count;
	uint32 rdwr_count;
	//mutex_t mtx;
	struct sockaddr_in addr;
	struct async_ov read_ov;
	struct async_ov write_ov;
	struct double_buffer input;
	struct double_buffer output;
	struct service *svc;
	struct protocol *proto;
	void *proto_handle;
};

static struct slab_cache *l_connections = NULL;
static struct slab_cache *l_input_buffers = NULL;
static struct slab_cache *l_output_buffers = NULL;
static uint32 l_input_size;
static uint32 l_output_size;

static void connection_create_buffers(struct connection *c){
	// input buffers
	c->input.length[0] = 0;
	c->input.length[1] = 0;
	c->input.buffers[0] = slab_cache_alloc(l_input_buffers);
	c->input.buffers[1] = slab_cache_alloc(l_input_buffers);

	// output buffers
	c->output.length[0] = 0;
	c->output.length[1] = 0;
	c->output.buffers[0] = slab_cache_alloc(l_output_buffers);
	c->output.buffers[1] = slab_cache_alloc(l_output_buffers);
}

static void connection_destroy_buffers(struct connection *c){
	slab_cache_free(l_input_buffers, c->input.buffers[0]);
	slab_cache_free(l_input_buffers, c->input.buffers[1]);
	slab_cache_free(l_output_buffers, c->output.buffers[0]);
	slab_cache_free(l_output_buffers, c->output.buffers[1]);
}

static struct connection *connection_create(SOCKET s,
		struct sockaddr_in *addr,
		struct service *svc){
	struct connection *c = slab_cache_alloc(l_connections);
	c->s = s;
	c->flags = CONNECTION_NEW;
	c->ref_count = 1;
	c->rdwr_count = 0;
	mutex_init(&c->mtx);
	if(addr != NULL)
		memcpy(&c->addr, addr, sizeof(struct sockaddr_in));
	else
		memset(&c->addr, 0, sizeof(struct sockaddr_in));

	// create i/o buffers
	connection_create_buffers(c);

	// service & protocol
	c->svc = svc;
	c->proto = NULL;
	c->proto_handle = NULL;
}

static void connection_destroy(struct connection *c){
	if(c->s != INVALID_SOCKET)
		closesocket(c->s);
	mutex_destroy(&c->mtx);
	connection_destroy_buffers(c);
	slab_cache_free(l_connections, c);
}

bool connmgr_init(void){
	uint32 slab_slots = config_geti("conn_slots_per_slab");
	l_input_size = config_geti("conn_input_size");
	l_output_size = config_geti("conn_output_size");

	// a slab_cache_create would result in an abort so we don't
	// need to check for the results here
	l_connections = slab_cache_create(slab_slots, sizeof(struct connection));
	l_input_buffers = slab_cache_create(2*slab_slots, l_input_size);
	l_output_buffers = slab_cache_create(2*slab_slots, l_output_size);

	// still lets return a boolean just in case
	return true;
}

void connmgr_shutdown(void){
	slab_cache_destroy(l_output_buffers);
	slab_cache_destroy(l_input_buffers);
	slab_cache_destroy(l_connections);
}


void connmgr_start_connection(SOCKET s,
		struct sockaddr_in *addr,
		struct service *svc){
	struct connection *conn;
	conn = connection_create(s, addr, svc);
	ASSERT(svc->num_protocols > 0);
	if(svc->protocols[0].sends_first){
		conn->proto = &svc->protocols[0];
		conn->proto_handle = conn->proto->create_handle(conn);
		conn->proto->on_connect(conn, conn->proto_handle);
	}

	//@TODO: start reading
}


/*
static void connection_close(struct connection *c){
	ASSERT(c->s != INVALID_SOCKET);
	// if the connection has no pending work, it's socket will be
	// closed here, else it'll be closed when all pending work
	// has been processed
	if(c->pending_work == 0){
		closesocket(c->s);
		c->s = INVALID_SOCKET;
	}else{
		c->flags |= CONNECTION_CLOSING;
		CancelIoEx((HANDLE)c->s, NULL);
	}
}
*/
#endif //0
