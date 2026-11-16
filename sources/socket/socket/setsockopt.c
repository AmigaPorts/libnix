#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int setsockopt(int s, int level, int name, const void *val, socklen_t valsize)
{ struct SocketSettings *lss = _lx_get_socket_settings();
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_SetSockOpt(fp->lx_sock,level,name,val, valsize);
  return 0;
}
