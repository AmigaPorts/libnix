/* fcntl() and ioctl() on files: fcntl() reports the access mode, keeps
   O_APPEND and O_NONBLOCK, accepts FD_CLOEXEC, duplicates with F_DUPFD, and
   fails with EINVAL for unknown commands; ioctl() fails with ENOTTY.  Both
   fail with EBADF on a closed descriptor.  fcntl_ioctl_socket runs the same checks against the
   socket library's versions, and adds its own on a socket (EXTRA_CHECKS).  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define NAME "T:libnix-fcntl-test"

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
  long n = 0;
  int fd, fl, d1, d2, p[2];

  fd = open(NAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  check(fd >= 0, "open for writing");
  if (fd < 0)
    return 20;

  check(fcntl(fd, F_GETFD) == 0, "F_GETFD is 0");
  check(fcntl(fd, F_SETFD, FD_CLOEXEC) == 0, "F_SETFD FD_CLOEXEC succeeds");

  fl = fcntl(fd, F_GETFL);
  check(fl != -1 && (fl & O_ACCMODE) == O_WRONLY, "F_GETFL is O_WRONLY");
  check(fl != -1 && !(fl & O_APPEND), "F_GETFL has no O_APPEND");
  check((fcntl(STDIN_FILENO, F_GETFL) & O_ACCMODE) == O_RDONLY, "stdin is O_RDONLY");

  /* with O_APPEND, a write after seeking back still goes to the end */
  check(write(fd, "abc", 3) == 3, "write abc");
  check(fcntl(fd, F_SETFL, fl | O_APPEND) == 0, "F_SETFL O_APPEND succeeds");
  check(fcntl(fd, F_GETFL) & O_APPEND, "F_GETFL has O_APPEND");
  lseek(fd, 0, SEEK_SET);
  check(write(fd, "de", 2) == 2, "write de");
  check(fcntl(fd, F_SETFL, fl) == 0, "F_SETFL without O_APPEND succeeds");
  check(!(fcntl(fd, F_GETFL) & O_APPEND), "F_GETFL has no O_APPEND again");

  /* O_NONBLOCK has no effect on a file, but F_GETFL reports it */
  check(fcntl(fd, F_SETFL, fl | O_NONBLOCK) == 0, "F_SETFL O_NONBLOCK on a file succeeds");
  check(fcntl(fd, F_GETFL) & O_NONBLOCK, "F_GETFL has O_NONBLOCK");
  check(fcntl(fd, F_SETFL, fl) == 0 && !(fcntl(fd, F_GETFL) & O_NONBLOCK),
        "F_SETFL clears O_NONBLOCK");

  errno = 0;
  check(fcntl(fd, F_DUPFD, -1) == -1 && errno == EINVAL, "F_DUPFD below 0 fails with EINVAL");
  errno = 0;
  check(fcntl(fd, 9999) == -1 && errno == EINVAL, "an unknown command fails with EINVAL");

  errno = 0;
  check(ioctl(fd, FIONBIO, &n) == -1 && errno == ENOTTY, "ioctl on a file fails with ENOTTY");

  close(fd);

  /* F_DUPFD takes the lowest free descriptor from its argument on, and the
     copy shares the file position and outlives the original */
  fd = open(NAME, O_RDONLY);
  check(fd >= 0 && read(fd, buf, 2) == 2 && !memcmp(buf, "ab", 2), "the file starts with ab");
  d1 = fcntl(fd, F_DUPFD, 10);
  check(d1 == 10, "F_DUPFD 10 gives 10");
  d2 = fcntl(fd, F_DUPFD, 10);
  check(d2 == 11, "F_DUPFD 10 again gives 11");
  close(fd);
  check(read(d1, buf, sizeof buf) == 3 && !memcmp(buf, "cde", 3),
        "the copy reads on after the original is closed");
  close(d1);
  close(d2);

  errno = 0;
  check(fcntl(fd, F_GETFL) == -1 && errno == EBADF, "fcntl on a closed fd fails with EBADF");
  errno = 0;
  check(ioctl(fd, FIONBIO, &n) == -1 && errno == EBADF, "ioctl on a closed fd fails with EBADF");
  errno = 0;
  check(fcntl(fd, F_DUPFD, 10) == -1 && errno == EBADF, "F_DUPFD of a closed fd fails with EBADF");

  /* a pipe's ends report their own access modes, not those of the files
     that had their descriptors before */
  d1 = open(NAME, O_RDONLY);
  d2 = open(NAME, O_RDONLY | O_APPEND);
  close(d1);
  close(d2);
  if (pipe(p) == 0)
    {
      check(fcntl(p[0], F_GETFL) == O_RDONLY, "the read end of a pipe is O_RDONLY");
      check(fcntl(p[1], F_GETFL) == O_WRONLY, "the write end of a pipe is O_WRONLY");
      check(write(p[1], "x", 1) == 1 && read(p[0], buf, 1) == 1 && buf[0] == 'x',
            "a byte goes through the pipe");
      close(p[0]);
      close(p[1]);
    }
  else
    printf("no PIPE:, the pipe checks skipped\n");

  remove(NAME);
#ifdef EXTRA_CHECKS
  EXTRA_CHECKS();
#endif
  printf("%d failures\n", failures);
  return failures;
}
