#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "def.h"
#include <functional>

struct Dispatcher;
using Task = std::function<void(void)>;

void dispatcher_init(void);
void dispatcher_shutdown(void);
void dispatcher_add(const Task &task);
void dispatcher_add(Task &&task);

#endif //DISPATCHER_H_
