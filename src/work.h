#ifndef WORK_H_
#define WORK_H_

#include <functional>

using Work = std::function<void(void)>;

void work_init(void);
void work_shutdown(void);
void work_dispatch(Work wrk);
void work_dispatch_array(int count, bool single, Work *wrk);

#endif //WORK_H_
