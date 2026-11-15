#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{ struct SocketSettings *lss;
  StdFileDes *fp;
  int rc;

  if ((fp=_lx_fhfromfd(s))) {
    switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {
      case LX_AMITCP:
        rc = TCP_Bind(fp->lx_sock, name, namelen);
      break;

      default:
        rc = -1;
      break;
    }
  }
  else rc = -1;

  return rc;
}
