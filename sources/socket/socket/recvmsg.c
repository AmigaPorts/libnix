#include <sys/types.h>
#include <sys/socket.h>
//
#include "socket.h"

ssize_t recvmsg(int s, struct msghdr *msg, int flags)
{ struct SocketSettings *lss;
  StdFileDes *fp = _lx_fhfromfd(s);

  lss = _lx_get_socket_settings();
  if (lss->lx_network_type == LX_AMITCP)
    return TCP_RecvMsg(fp->lx_sock,msg,flags);
  return -1;
}
