#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "../def.h"
#include <atomic>

#define MESSAGE_BUFFER_LEN 4096

class Message{
// this is a low-level class so making all attributes public is fine
// as adding getters and setters here would just increase code bloat
public:
	long readpos;
	long length;
	std::atomic_flag busy;
	uint8 buffer[MESSAGE_BUFFER_LEN];

	// message control
	bool try_acquire(void);
	void release(void);

	// peek data
	uint32 peek_u32(void);

	// get data
	uint8	get_byte(void);
	uint16	get_u16(void);
	uint32	get_u32(void);
	void	get_str(char *buf, uint16 buflen);

	// add data
	void	add_byte(uint8 val);
	void	add_u16(uint16 val);
	void	add_u32(uint32 val);
	void	add_str(const char *buf, uint16 buflen);

	// reverse add
	void	radd_byte(uint8 val);
	void	radd_u16(uint16 val);
	void	radd_u32(uint32 val);
	void	radd_str(const char *buf, uint16 buflen);
};

#endif //MESSAGE_H_
