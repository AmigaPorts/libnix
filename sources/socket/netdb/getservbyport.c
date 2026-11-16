#include <netdb.h>
//
#include "socket.h"

struct servent *getservbyport(int port, const char *proto)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetServByPort(port,proto);
  return NULL;
}
