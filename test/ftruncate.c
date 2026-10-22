/* ftruncate(): must return 0 on success, as POSIX requires, not the new
   file size (only a truncation to size 0 would have hidden that bug). Also
   checks that truncating to a non-zero size actually changes the file's
   length, and that a bad file descriptor fails with EBADF.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define NAME "T:libnix-ftruncate-test"

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
  int fd;
  off_t len;

  fd = open(NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
  check(fd >= 0, "open for writing");
  if (fd < 0)
    return 20;

  check(write(fd, "0123456789", 10) == 10, "write 10 bytes");

  /* truncating to a non-zero size must return 0, not the new size */
  check(ftruncate(fd, 4) == 0, "ftruncate to 4 returns 0");
  len = lseek(fd, 0, SEEK_END);
  check(len == 4, "file is 4 bytes after truncating to 4");

  /* extending the file must also return 0 */
  check(ftruncate(fd, 20) == 0, "ftruncate to 20 returns 0");
  len = lseek(fd, 0, SEEK_END);
  check(len == 20, "file is 20 bytes after truncating to 20");

  close(fd);

  /* a closed/bad descriptor must fail with EBADF */
  errno = 0;
  check(ftruncate(fd, 0) == -1, "ftruncate on a closed fd fails");
  check(errno == EBADF, "errno is EBADF after ftruncate on a closed fd");

  remove(NAME);
  printf("%d failures\n", failures);
  return failures;
}
