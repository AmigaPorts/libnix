#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_Connect(fp->lx_sock, name,namelen);
  return -1;
}
