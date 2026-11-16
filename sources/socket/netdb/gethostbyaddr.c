#include <netdb.h>
//
#include "socket.h"

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetHostByAddr(addr,len,type);
  return NULL;
}
