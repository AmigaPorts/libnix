#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>
#include <sys/param.h>
#include <stdlib.h>
#include "amigapath.h"

extern void __seterrno(void);

/* Resolve name to an absolute AmigaDOS path.  A NULL resolved buffer is
 * allocated with malloc, as POSIX allows. */
char *realpath(const char *name, char *resolved) {
	BPTR lock;
	char *buf;

	if ((name = __amigapath(name)) == NULL)
		return NULL;

	lock = Lock((CONST_STRPTR)name, ACCESS_READ);
	if (lock == 0) {
		__seterrno();
		return NULL;
	}

	buf = resolved ? resolved : malloc(MAXPATHLEN);
	if (buf == NULL) {
		UnLock(lock);
		errno = ENOMEM;
		return NULL;
	}

	if (!NameFromLock(lock, (STRPTR)buf, MAXPATHLEN)) {
		__seterrno();
		UnLock(lock);
		if (buf != resolved)
			free(buf);
		return NULL;
	}

	UnLock(lock);
	return buf;
}
