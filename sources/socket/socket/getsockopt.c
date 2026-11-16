#include "stdio.h"
#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int getsockopt(int s, int level, int name, void *val, socklen_t *valsize)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetSockOpt(fp->lx_sock,level,name,(char *)val, (int*)valsize);
  return 0;
}
