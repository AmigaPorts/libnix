#include <errno.h>
#include "stdio.h"

/* Only sockets take ioctl() requests, in the socket library's ioctl(),
 * which comes first.  */
int __stdargs
ioctl (int fd, unsigned long cmd, ...)
{
  errno = _lx_fhfromfd(fd) ? ENOTTY : EBADF;
  return -1;
}
