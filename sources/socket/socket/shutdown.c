#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

int shutdown(int s, int how)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_ShutDown(fp->lx_sock,how);
  return 0;
}
