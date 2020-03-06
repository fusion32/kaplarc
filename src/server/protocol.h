#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_
#include "../def.h"

typedef enum protocol_status{
	// default behaviour, the connection will keep going
	PROTO_OK = 0,
	// the connection will stop reading (only used from
	// within the on_recv_* handlers and will trigger an
	// assertion if used elsewhere)
	PROTO_STOP_READING,
	// the connection will be closed on return
	PROTO_CLOSE,
	// the connection will be aborted on return
	PROTO_ABORT,
} protocol_status_t;

struct connection;
struct protocol{
	/* name of the protocol: used for debugging */
	char *name;
	/* sends the first message: cannot be coupled
	 * with any other protocol
	 */
	bool sends_first;
	/* if the protocol doesn't send the first message,
	 * it may be coupled with other protocols so it
	 * needs to identify the first message to know if
	 * it owns the connection or not
	 */
	bool (*identify)(uint8 *data, uint32 datalen);

	/* protocols will interact with the server through
	 * the use of these handles
	 */
	bool (*create_handle)(struct connection *conn, void **handle);
	void (*destroy_handle)(struct connection *conn, void *handle);

	/* events related to the protocol */
	void (*on_close)(struct connection *conn, void *handle);
	protocol_status_t (*on_connect)(
		struct connection *conn, void *handle);
	protocol_status_t (*on_write)(
		struct connection *conn, void *handle);
	protocol_status_t (*on_recv_message)(
		struct connection *conn, void *handle,
		uint8 *data, uint32 datalen);
	protocol_status_t (*on_recv_first_message)(
		struct connection *conn, void *handle,
		uint8 *data, uint32 datalen);
};

#endif //PROTOCOL_H_
