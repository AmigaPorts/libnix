#include <unistd.h>

asm(".global _getuid; _getuid = _getppid");
asm(".global _geteuid; _geteuid = _getppid");
asm(".global _getgid; _getgid = _getppid");
asm(".global _getegid; _getegid = _getppid");
pid_t getppid(void)
{
  return 0; /* init ;-) */
}
