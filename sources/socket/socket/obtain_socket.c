#include "socket.h"

int obtain_socket(long id, int inet, int stream, int protocol)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return TCP_ObtainSocket(id, inet, stream, protocol);
  return -1;
}
