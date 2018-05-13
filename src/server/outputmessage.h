#ifndef SERVER_OUTPUTMESSAGE_H_
#define SERVER_OUTPUTMESSAGE_H_

#include <memory>
class Message;
using OutputMessage = std::unique_ptr<
	Message, void(*)(Message*)>;
OutputMessage output_message(size_t capacity);

#endif //SERVER_OUTPUTMESSAGE_H_
