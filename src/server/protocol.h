#ifndef SERVER_PROTOCOL_H_
#define SERVER_PROTOCOL_H_

class Connection;
class Message;
struct Protocol{
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
	bool (*identify)(Message *first);

	/* protocols will interact with the server through
	 * the use of these handles
	 */
	void *(*create_handle)(Connection *conn);
	void (destroy_handle)(Connection *conn, void *handle);

	/* events related to the protocol */
	void (*on_connect)(Connection *conn, void *handle);
	void (*on_close)(Connection *conn, void *handle);
	void (*on_recv_message)(Connection *conn, void *handle, Message *msg);
	void (*on_recv_first_message)(Connection *conn, void *handle, Message *msg);
};

#endif //PROTOCOL_H_
