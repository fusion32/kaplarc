#ifndef WORK_H_
#define WORK_H_

#include <functional>

namespace kp{

using work = std::function<void(void)>;

} //namespace

void work_init(void);
void work_shutdown(void);
void work_dispatch(kp::work wrk);
void work_dispatch_array(int count, bool single, kp::work *wrk);

#endif //WORK_H_
