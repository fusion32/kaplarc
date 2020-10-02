#ifndef KAPLAR_SERVER_PROTOCOL_H_
#define KAPLAR_SERVER_PROTOCOL_H_ 1
#include "../common.h"

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

struct protocol{
	/* NOTES: */
	/* `identify` is optional and won't be used if sends_first == true */
	/* `on_connect` is optional and won't be used if sends_first == false */
	/* `on_write` will only be called if the write succeeds. If theres a
	 * write error, the server will try to resend the message a few more
	 * times before dropping the connection. Any userdata associated with
	 * writes should be handled inside the `on_close` callback as well in
	 * case of write failures.
	 */

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

	/* events related to the protocol */
	bool (*on_assign_protocol)(uint32 conn);
	void (*on_close)(uint32 conn);
	protocol_status_t (*on_connect)(uint32 conn);
	protocol_status_t (*on_write)(uint32 conn);
	protocol_status_t (*on_recv_message)(uint32 conn, uint8 *data, uint32 datalen);
	protocol_status_t (*on_recv_first_message)(uint32 conn, uint8 *data, uint32 datalen);
};

#endif //KAPLAR_SERVER_PROTOCOL_H_
