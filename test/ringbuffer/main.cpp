#include "../../src/ringbuffer.h"
#include "../../src/log.h"

int main(int argc, char **argv)
{
	RingBuffer<int, 32> rb;

	for(int j = 1; j <= 10; j++){
		LOG("=======================");
		LOG("j = %d", j);
		LOG("=======================");
		// insert elements
		for(int i = 0; i < 4*j; i++)
			rb.push(i);

		// pop until exhaust ringbuffer
		int *ptr;
		while((ptr = rb.pop()) != nullptr)
			LOG("pop = %d", *ptr);
	}

	return 0;
}