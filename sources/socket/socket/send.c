#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t send(int s, const void *buf, size_t len, int flags)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_Send(fp->lx_sock,buf,len,flags);
  return -1;
}
