#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int listen(int s, int backlog)
{ struct SocketSettings *lss;
  register StdFileDes *fp;
  int rc;

  if ((fp=_lx_fhfromfd(s))) {
    lss = _lx_get_socket_settings();
    if (lss->lx_network_type == LX_AMITCP)
      rc = TCP_Listen(fp->lx_sock, backlog);
    else
      rc = -1;
  }
  else rc = -1;

  return rc;
}
