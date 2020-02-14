#if 0
#include "protocol.h"
#include "buffer_util.h"
#include "hash.h"
#include "mem.h"
#include "server.h"
#include "packed_data.h"
#include "tibia_rsa.h"
#include "crypto/xtea.h"

/* OUTPUT BUFFER */
#define GAME_HANDLE_SIZE sizeof(struct game_handle)

/* PROTOCOL DECL */
static bool identify(uint8 *data, uint32 datalen);
static bool create_handle(struct connection *c, void **handle);
static void destroy_handle(struct connection *c, void *handle);
static void on_close(struct connection *c, void *handle);
static protocol_status_t on_connect(
	struct connection *c, void *handle);
static protocol_status_t on_write(
	struct connection *c, void *handle);
static protocol_status_t on_recv_message(
	struct connection *c, void *handle,
	uint8 *data, uint32 datalen);
static protocol_status_t on_recv_first_message(
	struct connection *c, void *handle,
	uint8 *data, uint32 datalen);
struct protocol protocol_login = {
	.name = "game",
	.sends_first = true,
	.identify = identify,

	.create_handle = create_handle,
	.destroy_handle = destroy_handle,
	.on_close = on_close,
	.on_connect = on_connect,
	.on_write = on_write,
	.on_recv_message = on_recv_message,
	.on_recv_first_message = on_recv_first_message,
};
#endif
