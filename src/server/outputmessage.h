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

struct OutputMessage{
	uint16 pool_idx;
	uint16 array_idx;
	uint32 array_slot;
	Message *msg;
};

void output_message_acquire(MessageCapacity capacity, OutputMessage *out);
void output_message_release(OutputMessage *out);

#endif //SERVER_OUTPUTMESSAGE_H_
