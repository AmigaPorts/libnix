/* fcntl() and ioctl() on files: fcntl() reports the access mode, keeps
   O_APPEND and accepts FD_CLOEXEC, and fails with EINVAL for O_NONBLOCK and
   unknown commands; ioctl() fails with ENOTTY.  Both fail with EBADF on a
   closed descriptor.  fcntl_ioctl_socket runs the same checks against the
   socket library's versions.  */

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
  int fd, fl;

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

  errno = 0;
  check(fcntl(fd, F_SETFL, O_NONBLOCK) == -1 && errno == EINVAL,
        "F_SETFL O_NONBLOCK on a file fails with EINVAL");
  errno = 0;
  check(fcntl(fd, 9999) == -1 && errno == EINVAL, "an unknown command fails with EINVAL");

  errno = 0;
  check(ioctl(fd, FIONBIO, &n) == -1 && errno == ENOTTY, "ioctl on a file fails with ENOTTY");

  close(fd);

  fd = open(NAME, O_RDONLY);
  check(fd >= 0 && read(fd, buf, sizeof buf) == 5 && !memcmp(buf, "abcde", 5),
        "the file is abcde");
  close(fd);

  errno = 0;
  check(fcntl(fd, F_GETFL) == -1 && errno == EBADF, "fcntl on a closed fd fails with EBADF");
  errno = 0;
  check(ioctl(fd, FIONBIO, &n) == -1 && errno == EBADF, "ioctl on a closed fd fails with EBADF");

  remove(NAME);
  printf("%d failures\n", failures);
  return failures;
}
