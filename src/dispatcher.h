#ifndef DQUEUE_H_
#define DQUEUE_H_

#include <functional>
#include "def.h"
class Dispatcher;
using Task = std::function<void(void)>;
bool dispatcher_create(Dispatcher **d, uint32 capacity);
void dispatcher_destroy(Dispatcher *d);
void dispatcher_add(Dispatcher *d, const Task &task);
void dispatcher_add(Dispatcher *d, Task &&task);

#endif //DQUEUE_H_
