#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);
  int rc;

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {
    case LX_AMITCP:
      rc = TCP_Connect(fp->lx_sock, name,namelen);
    break;

    default:
      rc = -1;
    break;
  }

  return rc;
}
