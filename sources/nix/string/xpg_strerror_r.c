#include <errno.h>
#include <string.h>

extern const char ** __sys_errlist;
extern const int __sys_nerr;

/* POSIX strerror_r, under the name glibc and newlib give it: the newlib
   string.h installed in sys-include renames strerror_r to __xpg_strerror_r
   unless _GNU_SOURCE is defined, so objects built against those headers
   (libstdc++'s system_error.o among them) reference this symbol. */
int __xpg_strerror_r(int eno, char *buffer, size_t buflen)
{ int valid = (eno >= 0 && eno < __sys_nerr);
  const char *msg = valid ? __sys_errlist[eno] : "Unknown error";
  size_t len = strlen(msg);

  if (buffer == NULL || buflen == 0)
    return ERANGE;
  if (len >= buflen)
  { memcpy(buffer, msg, buflen - 1);
    buffer[buflen - 1] = '\0';
    return ERANGE; }
  memcpy(buffer, msg, len + 1);
  return valid ? 0 : EINVAL;
}
