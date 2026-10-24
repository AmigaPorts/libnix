/* fstat(): a regular file reports S_IFREG and its size; a bad file
   descriptor fails with EBADF, and a null buffer with EFAULT.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAME "T:libnix-fstat-test"

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
  struct stat st;
  int fd;

  fd = open(NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
  check(fd >= 0, "open for writing");
  if (fd < 0)
    return 20;

  check(write(fd, "0123456789", 10) == 10, "write 10 bytes");
  check(fstat(fd, &st) == 0, "fstat on the file succeeds");
  check(S_ISREG(st.st_mode), "the file is S_IFREG");
  check(st.st_size == 10, "the file is 10 bytes");

  errno = 0;
  check(fstat(fd, NULL) == -1, "fstat with a null buffer fails");
  check(errno == EFAULT, "errno is EFAULT after fstat with a null buffer");

  close(fd);

  /* a closed/bad descriptor must fail with EBADF */
  errno = 0;
  check(fstat(fd, &st) == -1, "fstat on a closed fd fails");
  check(errno == EBADF, "errno is EBADF after fstat on a closed fd");

  remove(NAME);
  printf("%d failures\n", failures);
  return failures;
}
