#include <netdb.h>
//
#include "socket.h"

struct netent *getnetbyaddr(in_addr_t net, int type)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetNetByAddr(net,type);
  return NULL;
}
