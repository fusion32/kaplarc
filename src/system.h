#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "def.h"
int64	sys_tick_count(void);
int	sys_cpu_count(void);
void	*sys_aligned_alloc(size_t alignment, size_t size);
void	*sys_aligned_realloc(void *ptr, size_t alignment, size_t size);
void	sys_aligned_free(void *ptr);

#endif //SYSTEM_H_
