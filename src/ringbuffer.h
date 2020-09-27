#ifndef KAPLAR_RINGBUFFER_H_
#define KAPLAR_RINGBUFFER_H_ 1

#include "common.h"

// STATIC REGULAR RINGBUFFER
#define DECL_RINGBUFFER(ident, type, capacity)				\
	static const int ident##_po2_check[IS_POWER_OF_TWO(capacity)];	\
	static const uint32 ident##_mask = capacity - 1;		\
	static uint32 ident##_readpos = 0;				\
	static uint32 ident##_writepos = 0;				\
	static type ident##_buffer[capacity];
#define RINGBUFFER_EMPTY(ident) (ident##_readpos == ident##_writepos)
#define RINGBUFFER_FULL(ident) ((ident##_writepos - ident##_readpos) > ident##_mask)
#define RINGBUFFER_UNCHECKED_PUSH(ident) (&ident##_buffer[ident##_writepos++ & ident##_mask])
#define RINGBUFFER_UNCHECKED_POP(ident) (&ident##_buffer[ident##_readpos++ & ident##_mask])

// STATIC TASK RINGBUFFER
struct task{
	void (*fp)(void*);
	void *arg;
};
#define DECL_TASK_RINGBUFFER(ident, capacity) \
	DECL_RINGBUFFER(ident, struct task, capacity)


#endif //KAPLAR_RINGBUFFER_H_
