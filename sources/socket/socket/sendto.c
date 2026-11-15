#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);
  int rc;

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {
    case LX_AMITCP:
      rc = TCP_SendTo(fp->lx_sock,buf,len,flags,to,tolen);
    break;

    default:
      rc = -1;
    break;
  }

  return rc;
}
