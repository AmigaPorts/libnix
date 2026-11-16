#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_SendMsg(fp->lx_sock,msg,flags);
  return -1;
}
