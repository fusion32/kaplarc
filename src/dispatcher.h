#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "def.h"
#include <functional>

struct Dispatcher;
using Task = std::function<void(void)>;

Dispatcher *dispatcher_create(void);
void dispatcher_destroy(Dispatcher *d);
void dispatcher_add(Dispatcher *d, const Task &task);
void dispatcher_add(Dispatcher *d, Task &&task);

#endif //DISPATCHER_H_
