#include "stdio.h"
#include <fcntl.h>
#include <stdarg.h>
#include <sys/ioctl.h>

/* Replaces libnix's fcntl() to set O_NONBLOCK on a socket through FIONBIO,
   which records it for F_GETFL.  */
__stdargs int fcntl(int fd, int cmd, ... /* arg */ )
{
  StdFileDes *sfd = _lx_fhfromfd(fd);
  va_list va;
  long arg;

  va_start(va, cmd);
  arg = va_arg(va, long);
  va_end(va);

  if (sfd && (sfd->lx_flags & LX_SOCKET) && cmd == F_SETFL) {
    long on = (arg & O_NONBLOCK) != 0;
    return ioctl(fd, FIONBIO, &on);
  }
  return __fcntl(fd, cmd, arg);
}
