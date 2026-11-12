/* fcntl_ioctl.c, linked with the socket library's fcntl() and ioctl(), and
   their checks on a socket: O_NONBLOCK set by fcntl() or by ioctl(FIONBIO)
   and reported by F_GETFL, kept by a read-modify-write of the flags, and in
   effect: accept() on a listening socket does not wait.  Without a TCP/IP
   stack (the simulators have none), the socket checks are skipped.  */

static void socket_checks(void);
#define EXTRA_CHECKS socket_checks
#include "fcntl_ioctl.c"

#include <netinet/in.h>
#include <sys/socket.h>

static void socket_checks(void)
{
  struct sockaddr_in sin;
  long on = 0;
  int s, fl;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    {
      printf("no TCP/IP stack: socket checks skipped\n");
      return;
    }

  fl = fcntl(s, F_GETFL);
  check(fl == O_RDWR, "a new socket is O_RDWR and blocking");
  check(fcntl(s, F_SETFL, fl | O_NONBLOCK) == 0, "F_SETFL O_NONBLOCK on a socket succeeds");
  check(fcntl(s, F_GETFL) & O_NONBLOCK, "F_GETFL on the socket has O_NONBLOCK");
  /* another layer adds a flag of its own */
  check(fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_APPEND) == 0
        && (fcntl(s, F_GETFL) & O_NONBLOCK),
        "a read-modify-write of the flags keeps O_NONBLOCK");

  memset(&sin, 0, sizeof sin);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1: no INADDR_LOOPBACK */
  if (bind(s, (struct sockaddr *) &sin, sizeof sin) == 0 && listen(s, 1) == 0)
    {
      errno = 0;
      check(accept(s, NULL, NULL) == -1 && (errno == EWOULDBLOCK || errno == EAGAIN),
            "accept() on a non-blocking socket fails with EWOULDBLOCK");
    }
  else
    printf("no loopback interface: the accept() check skipped\n");

  check(ioctl(s, FIONBIO, &on) == 0 && !(fcntl(s, F_GETFL) & O_NONBLOCK),
        "ioctl(FIONBIO, 0) clears O_NONBLOCK for F_GETFL");
  close(s);
}
