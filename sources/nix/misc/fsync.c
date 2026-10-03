#include <unistd.h>

asm(".global _fdatasync; _fdatasync = _fsync");
int fsync(int fd) {
	return 0;
}
