#ifndef WORK_H_
#define WORK_H_

#include <functional>

using Work = std::function<void(void)>;

void work_init(void);
void work_shutdown(void);
void work_dispatch(Work wrk);
void work_multi_dispatch(int count, const Work &wrk);
void work_multi_dispatch(int count, const Work *wrk);

#endif //WORK_H_
