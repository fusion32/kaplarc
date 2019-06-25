#ifndef SERVER_OUTPUTMESSAGE_H_
#define SERVER_OUTPUTMESSAGE_H_

#include "../def.h"
#include "message.h"

enum MessageCapacity: size_t {
	MESSAGE_CAPACITY_256	= (1 << 8),
	MESSAGE_CAPACITY_1K	= (1 << 10),
	MESSAGE_CAPACITY_4K	= (1 << 12),
	MESSAGE_CAPACITY_16K	= (1 << 14),
};

class OutputMessage{
public:
	uint16 pool_idx;
	uint16 array_idx;
	uint32 array_slot;
	Message *msg;
};

void acquire_output_message(MessageCapacity capacity, OutputMessage *msg);
void release_output_message(OutputMessage *msg);

#endif //SERVER_OUTPUTMESSAGE_H_
