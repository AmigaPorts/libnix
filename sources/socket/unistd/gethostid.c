#include <unistd.h>
//
#include "socket.h"

long gethostid(void)
{ struct SocketSettings *lss;

  switch (lss=_lx_get_socket_settings(),lss->lx_network_type) {
    case LX_AMITCP:
      return (long)TCP_GetHostId();

    default:
      return -1;
  }
}
