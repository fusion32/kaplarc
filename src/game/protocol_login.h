#ifndef PROTOCOL_LOGIN_H_
#define PROTOCOL_LOGIN_H_

#include "../server/protocol.h"
class ProtocolLogin: public Protocol{
private:
	void disconnect(const char *message, uint32 *xtea, bool checksum);

public:
	// protocol information
	static constexpr char	*name = "login";
	static constexpr bool	single = false;
	static bool		identify(Message *first);

	// protocol interface
	ProtocolLogin(Connection *conn);
	virtual ~ProtocolLogin(void) override;
	virtual void on_recv_first_message(Message *msg) override;
};

#endif //PROTOCOL_LOGIN_H_
