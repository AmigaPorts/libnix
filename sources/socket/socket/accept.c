#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int accept(int s, struct sockaddr *name, socklen_t *namelen)
{ struct SocketSettings *lss = _lx_get_socket_settings();
  StdFileDes *fp,*fp2;
  int rc;

  if ((fp=_lx_fhfromfd(s)) == NULL)
    return 0;

  if ((fp2=_create_socket(fp->lx_family, fp->lx_domain, fp->lx_protocol)) == NULL)
    return -1;

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    rc = TCP_Accept(fp->lx_sock, name, (int*)namelen);
  else
    rc = -1;

  if (rc >= 0) {
    fp2->lx_sock = rc; rc = fp2->lx_pos;
    /* the stack gives the new socket the listener's non-blocking state */
    fp2->lx_flags |= fp->lx_flags & LX_NONBLOCK;
  }
  else {
    fp2->lx_inuse = 0;
  }

  return rc;
}
