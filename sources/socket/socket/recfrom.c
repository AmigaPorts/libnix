#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_RecvFrom(fp->lx_sock,buf,len,flags, from, (int*)fromlen);
  return -1;
}
