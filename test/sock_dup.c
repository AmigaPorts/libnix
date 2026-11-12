/* dup(), dup2() and close() on sockets: a copy is a socket of its own, so
   it outlives the original, and a closed socket's descriptor is free again.
   Needs a TCP/IP stack, which volamos has with --net; skipped without one.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

/* The lowest free descriptor.  */
static int next_fd(void)
{
  int s = socket(AF_INET, SOCK_STREAM, 0);

  close(s);
  return s;
}

/* Whether a byte written to FROM arrives at TO.  */
static int passes(int from, int to, char c)
{
  char r = 0;

  return write(from, &c, 1) == 1 && read(to, &r, 1) == 1 && r == c;
}

int main(void)
{
  struct sockaddr_in addr;
  socklen_t len = sizeof addr;
  int ls, c, a, d, x, f, free_fd;

  ls = socket(AF_INET, SOCK_STREAM, 0);
  if (ls < 0)
    {
      printf("SKIP: no TCP/IP stack\n");
      return 0;
    }

  /* a connection over the loopback: c sends, a receives */
  memset(&addr, 0, sizeof addr);
  addr.sin_len = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  check(bind(ls, (struct sockaddr *)&addr, sizeof addr) == 0, "bind");
  check(listen(ls, 1) == 0, "listen");
  check(getsockname(ls, (struct sockaddr *)&addr, &len) == 0, "getsockname");
  c = socket(AF_INET, SOCK_STREAM, 0);
  check(connect(c, (struct sockaddr *)&addr, sizeof addr) == 0, "connect");
  a = accept(ls, NULL, NULL);
  check(a >= 0, "accept");
  if (failures)
    return failures;
  check(passes(c, a, 'a'), "send through the socket");

  /* the copy outlives the original, whose descriptor is free again */
  d = dup(c);
  check(d >= 0 && d != c, "dup");
  check(close(c) == 0, "close the original");
  check(write(c, "x", 1) == -1, "write to the closed descriptor fails");
  check(next_fd() == c, "the closed descriptor is free");
  check(passes(d, a, 'b'), "send through the copy after closing the original");

  /* dup2() onto an open socket replaces it and takes no other descriptor */
  x = socket(AF_INET, SOCK_STREAM, 0);
  check(x == c, "socket reuses the closed descriptor");
  free_fd = next_fd();
  check(dup2(d, x) == x, "dup2 onto an open socket");
  check(next_fd() == free_fd, "dup2 onto an open socket takes no other descriptor");
  check(passes(x, a, 'c'), "send through the dup2()");
  check(dup2(d, d) == d && passes(d, a, 'd'), "dup2 of a socket to itself");

  /* onto a free descriptor past the table's end, and onto an open file */
  check(dup2(d, 20) == 20 && passes(20, a, 'e'), "dup2 to 20");
  check(close(20) == 0, "close 20");
  f = open("NIL:", O_WRONLY);
  check(f >= 0 && dup2(d, f) == f && passes(f, a, 'f'), "dup2 onto an open file");
  check(close(f) == 0, "close the copy that replaced the file");
  check(next_fd() == free_fd, "the copies' descriptors are free again");

  /* as set_socket_stdio() does: onto stdin, then the original is closed */
  check(dup2(d, 0) == 0, "dup2 onto stdin");
  check(close(d) == 0, "close the original of stdin's copy");
  check(passes(0, a, 'g'), "send through stdin after closing the original");

  errno = 0;
  check(dup(d) == -1 && errno == EBADF, "dup of a closed socket fails with EBADF");
  errno = 0;
  check(dup2(d, x) == -1 && errno == EBADF, "dup2 of a closed socket fails with EBADF");
  check(passes(x, a, 'h'), "a failed dup2 leaves its target open");

  /* x, stdin's copy, a and ls are left to the exit code */
  printf("%d failures\n", failures);
  return failures;
}
