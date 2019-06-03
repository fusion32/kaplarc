#ifndef SERVER_MESSAGE_H_
#define SERVER_MESSAGE_H_

#include "../def.h"

class Message{
public:
	size_t capacity;
	size_t length;
	size_t readpos;
	uint8 buffer[];

	// messages should be created using these
	static size_t total_size(size_t capacity);
	static Message *create(size_t total_size);
	static Message *create_with_capacity(size_t capacity);
	static Message *takeon(void *mem, size_t mem_size);
	static void destroy(Message *msg);

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

#endif //SERVER_MESSAGE_H_
