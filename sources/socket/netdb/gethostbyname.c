#include <netdb.h>
//
#include "socket.h"

struct hostent *gethostbyname(const char *name)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetHostByName(name);
  return NULL;
}
