/* dup() and dup2(): a copy shares the original's file, which stays open
   until the last copy is closed, in any order, or until exit; dup2(fd, fd)
   leaves fd alone, dup2() onto an open descriptor or stdout replaces it, and
   a bad descriptor fails with EBADF.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NAME "T:libnix-dup-test"
#define NAME2 "T:libnix-dup-test2"

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
  int fd, d, d2, other, saved;

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

  /* dup2() onto an open descriptor, which then writes to the copied file */
  other = open(NAME2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  check(other >= 0, "open a second file");
  check(dup2(d2, other) == other, "dup2 onto an open descriptor");
  check(write(other, "e", 1) == 1, "write through the replaced descriptor");
  check(close(other) == 0, "close the replaced descriptor");

  /* dup2() onto stdout and back closes neither the file nor Output() */
  fflush(stdout);
  saved = dup(1);
  check(saved >= 0, "dup of stdout");
  check(dup2(d2, 1) == 1, "dup2 onto stdout");
  check(write(1, "f", 1) == 1, "write to the redirected stdout");
  check(dup2(saved, 1) == 1, "dup2 of the saved stdout back");
  check(close(saved) == 0, "close the saved stdout");
  check(write(d2, "g", 1) == 1, "write after restoring stdout");
  check(close(d2) == 0, "close the last copy");

  errno = 0;
  check(dup(fd) == -1 && errno == EBADF, "dup of a closed fd fails with EBADF");
  errno = 0;
  check(dup2(fd, 8) == -1 && errno == EBADF, "dup2 of a closed fd fails with EBADF");

  fd = open(NAME, O_RDONLY);
  check(fd >= 0 && read(fd, buf, sizeof buf) == 7 && !memcmp(buf, "abcdefg", 7),
        "the file is abcdefg");
  close(fd);
  fd = open(NAME2, O_RDONLY);
  check(fd >= 0 && read(fd, buf, sizeof buf) == 0, "the second file is empty");
  close(fd);

  remove(NAME);
  remove(NAME2);

  /* left open with a copy: the exit code closes the handle once */
  fd = open("NIL:", O_WRONLY);
  check(fd >= 0 && dup(fd) >= 0, "copies left open at exit");

  printf("%d failures\n", failures);
  return failures;
}
