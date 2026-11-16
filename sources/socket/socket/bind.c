#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{ struct SocketSettings *lss;
  StdFileDes *fp;
  int rc;

  if ((fp=_lx_fhfromfd(s))) {
    lss = _lx_get_socket_settings();
    if (lss->lx_network_type == LX_AMITCP)
      rc = TCP_Bind(fp->lx_sock, name, namelen);
    else
      rc = -1;
  }
  else rc = -1;

  return rc;
}
