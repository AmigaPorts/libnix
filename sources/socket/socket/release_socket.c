#include <unistd.h>
//
#include "socket.h"

/* TODO: fix me */
int release_socket(int s)
{ struct SocketSettings *lss;
  /* TODO: Finish me */
  /* dup the socket first, since for AmiTCP, we can only release once */
  int s2 = dup(s);
  /*StdFileDes *fp = _lx_fhfromfd(s);*/
  int rc = -1;

  if (s2 >= 0) {
    lss = _lx_get_socket_settings();
    if (lss->lx_network_type == LX_AMITCP) {
      rc = TCP_ReleaseSocket(s2, -1);
      TCP_CloseSocket(s2);
    }
  }

  return rc;
}
