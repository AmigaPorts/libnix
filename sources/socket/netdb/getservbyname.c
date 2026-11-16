#include <netdb.h>
//
#include "socket.h"

struct servent *getservbyname(const char *name, const char *proto)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetServByName(name,proto);
  return NULL;
}
