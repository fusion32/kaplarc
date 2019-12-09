#ifndef SERVER_MESSAGE_H_
#define SERVER_MESSAGE_H_

#include "../def.h"

struct Message{
	size_t capacity;
	size_t length;
	size_t readpos;
	uint8 buffer[];

	// delete constructor and destructor
	Message(void) = delete;
	~Message(void) = delete;

	// peek data
	uint8 peek_byte(void);
	uint16 peek_u16(void);
	uint32 peek_u32(void);

	// get data
	uint8	get_byte(void);
	uint16	get_u16(void);
	uint32	get_u32(void);
	void	get_str(char *buf, size_t buflen);

	// add data
	void	add_byte(uint8 val);
	void	add_u16(uint16 val);
	void	add_u32(uint32 val);
	void	add_str(const char *str);
	void	add_lstr(const char *buf, size_t buflen);

	// reverse add
	void	radd_byte(uint8 val);
	void	radd_u16(uint16 val);
	void	radd_u32(uint32 val);
	void	radd_str(const char *str);
	void	radd_lstr(const char *buf, size_t buflen);
};

size_t message_total_size(size_t capacity);
Message *message_create(size_t total_size);
Message *message_create_with_capacity(size_t capacity);
Message *message_takeon(void *mem, size_t mem_size);
void message_destroy(Message *msg);

#endif //SERVER_MESSAGE_H_
