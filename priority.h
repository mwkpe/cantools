/* Functions for handling POSIX thread scheduling policy/priority
 */


#ifndef PRIORITY_H
#define PRIORITY_H


#include <sched.h>
#include <pthread.h>
#include <thread>


namespace priority
{


bool set_realtime(std::thread::native_handle_type handle)
{
  sched_param sch;
  auto priority = sched_get_priority_max(SCHED_FIFO);
  sch.sched_priority = priority > 0 ? priority : 1;  // FIFO must be at least 1
  return pthread_setschedparam(handle, SCHED_FIFO, &sch) == 0;
}


bool is_realtime(std::thread::native_handle_type handle)
{
  sched_param sch;
  int policy;
  if (pthread_getschedparam(handle, &policy, &sch) == 0)
    return policy == SCHED_FIFO || policy == SCHED_RR;
  return false;
}


int current_priority(std::thread::native_handle_type handle)
{
  sched_param sch;
  int policy;
  if (pthread_getschedparam(handle, &policy, &sch) == 0)
    return sch.sched_priority;
  return -1;
}


}  // namespace priority


#endif  // PRIORITY_H
