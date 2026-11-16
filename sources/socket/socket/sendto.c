#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_SendTo(fp->lx_sock,buf,len,flags,to,tolen);
  return -1;
}
