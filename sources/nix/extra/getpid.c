#include <unistd.h>
#include <proto/exec.h>

/* There are no process ids; the task address is unique while the task
   lives.  Shifted, as in newlib, so the result stays positive even for
   memory above 2 GB (tasks are at least longword aligned).  */
pid_t getpid(void)
{
  return (pid_t)((unsigned long)FindTask(NULL) >> 2);
}
