#include <netdb.h>
//
#include "socket.h"

struct protoent *getprotobynumber(int proto)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_GetProtoByNumber(proto);
  return NULL;
}
