#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int getpeername(int s, struct sockaddr *asa, socklen_t *alen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetPeerName(fp->lx_sock,asa,(int *)alen);
  return -1;
}
