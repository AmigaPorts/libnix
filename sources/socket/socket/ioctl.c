#include "stdio.h"
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
//
#include "socket.h"

__stdargs int ioctl(int s, unsigned long cmd, ...)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);
  int rc;
  caddr_t data;
  va_list va;

  if (!fp) {
    errno = EBADF; return -1;
  }
  if (!(fp->lx_flags & LX_SOCKET)) {
    errno = ENOTTY; return -1;
  }

  /* ioctl(fd, request, arg): arg is the request's data pointer */
  va_start(va, cmd);
  data = va_arg(va, caddr_t);
  va_end(va);

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {

    case LX_AMITCP:
      rc = TCP_IoctlSocket(fp->lx_sock,cmd,data);
    break;

    default:
      rc = 0;
    break;
  }

  /* for fcntl()'s F_GETFL */
  if (rc == 0 && cmd == FIONBIO) {
    if (*(long *)data)
      fp->lx_flags |= LX_NONBLOCK;
    else
      fp->lx_flags &= ~LX_NONBLOCK;
  }

  return rc;
}
