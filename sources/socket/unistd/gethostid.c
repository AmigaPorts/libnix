#include <unistd.h>
//
#include "socket.h"

long gethostid(void)
{ struct SocketSettings *lss = _lx_get_socket_settings();

  if (lss->lx_network_type == LX_AMITCP)
    return (long)TCP_GetHostId();
  return -1;
}
