#include <dos/dos.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <unistd.h>
#include "amigapath.h"

extern void __seterrno(void);

int access(const char *name, int mode) {
	BPTR lock;

	if((name=__amigapath(name))==NULL)
	return -1;

	lock = Lock((CONST_STRPTR )name, mode&(O_WRONLY|O_RDWR)?ACCESS_WRITE:ACCESS_READ);
	if (lock != 0l) {
		UnLock(lock);
		return 0;
	} else {
		__seterrno();
		return -1;
	}
}
