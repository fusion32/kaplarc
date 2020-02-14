#ifndef MEM_H_
#define MEM_H_
#include "def.h"

bool mem_init(void);
void mem_shutdown(void);

void *mem_alloc(size_t size);
void mem_free(size_t size, void *mem);

#endif //MEM_H_
