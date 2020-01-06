#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_

struct connection;
struct packed_data;
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
	bool (*identify)(struct packed_data *first);

	/* protocols will interact with the server through
	 * the use of these handles
	 */
	void *(*create_handle)(struct connection *conn);
	void (*destroy_handle)(struct connection *conn, void *handle);

	/* events related to the protocol */
	void (*on_connect)(struct connection *conn, void *handle);
	void (*on_close)(struct connection *conn, void *handle);
	void (*on_recv_message)(struct connection *conn, void *handle, struct packed_data *msg);
	void (*on_recv_first_message)(struct connection *conn, void *handle, struct packed_data *msg);
};

#endif //PROTOCOL_H_
