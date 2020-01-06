#ifdef RINGBUFFER_H_
#	error "ringbuffer.h" must be included only once
#endif
#define RINGBUFFER_H_ 1

#include "def.h"

// check if the ringbuffer parameters are set
#if !defined(RB_CAPACITY) || !defined(RB_BUFFER)	\
  || !defined(RB_WRPOS) || !defined(RB_RDPOS)
#	error all ringbuffer parameters must be set
#endif

// check if RB_CAPACITY is a power of two
#if !IS_POWER_OF_TWO(RB_CAPACITY)
#	error RB_CAPACITY must be a power of two
#endif

// auxiliary definitions
#define RB__CAPACITY	(RB_CAPACITY)
#define RB__BUFFER	(RB_BUFFER)
#define RB__WRPOS	(RB_WRPOS)
#define RB__RDPOS	(RB_RDPOS)
#define RB__MASK	(RB__CAPACITY - 1)

// ringbuffer helpers
#define RB_EMPTY()	(RB__WRPOS == RB__RDPOS)
#define RB_FULL()	((RB__WRPOS - RB__RDPOS) >= RB__CAPACITY)
#define RB_PUSH()	(&RB__BUFFER[RB__WRPOS++ & RB__MASK])
#define RB_POP()	(&RB__BUFFER[RB__RDPOS++ & RB__MASK])
