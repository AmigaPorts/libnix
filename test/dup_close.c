/* dup() and dup2(): a copy shares the original's file, which stays open
   until the last copy is closed, in any order; dup2(fd, fd) leaves fd alone,
   and a bad descriptor fails with EBADF.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NAME "T:libnix-dup-test"

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

int main(void)
{
  char buf[8];
  int fd, d, d2;

  fd = open(NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  check(fd >= 0, "open for writing");
  if (fd < 0)
    return 20;

  /* the copy closes first */
  d = dup(fd);
  check(d >= 0 && d != fd, "dup");
  check(write(d, "a", 1) == 1, "write through the dup()");
  check(close(d) == 0, "close the dup()");
  check(write(fd, "b", 1) == 1, "write through the original after closing its copy");

  /* the original closes first */
  d2 = dup2(fd, 7);
  check(d2 == 7, "dup2 to 7");
  check(dup2(fd, fd) == fd, "dup2 of a descriptor to itself");
  check(write(fd, "c", 1) == 1, "write after dup2 to itself");
  check(close(fd) == 0, "close the original");
  check(write(d2, "d", 1) == 1, "write through the dup2() after closing the original");
  check(close(d2) == 0, "close the last copy");

  errno = 0;
  check(dup(fd) == -1 && errno == EBADF, "dup of a closed fd fails with EBADF");
  errno = 0;
  check(dup2(fd, 8) == -1 && errno == EBADF, "dup2 of a closed fd fails with EBADF");

  fd = open(NAME, O_RDONLY);
  check(fd >= 0 && read(fd, buf, sizeof buf) == 4 && !memcmp(buf, "abcd", 4),
        "the file is abcd");
  close(fd);

  remove(NAME);
  printf("%d failures\n", failures);
  return failures;
}
