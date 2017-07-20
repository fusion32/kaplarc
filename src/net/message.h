#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <atomic>
#include "../def.h"

#define MESSAGE_BUFFER_LEN 4096

class Message{
// this is a low level class anyways, so making
// the members public, will save us from adding
// getters and setters which wouldn't add anything
// to the table
public:
	long readpos;
	long length;
	std::atomic_flag busy;
	uint8 buffer[MESSAGE_BUFFER_LEN];

	// message control
	bool try_acquire(void);
	void release(void);

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
};

#endif //MESSAGE_H_
