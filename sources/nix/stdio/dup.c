#include "stdio.h"
#include <errno.h>

/*
 **
 */

extern StdFileDes *__allocfd();

int dup(int oldfd) {
	StdFileDes *neu, *sfd;
	int fd = -1;

	__GLOBAL_STDIO_LOCK;
	if (!(sfd = _lx_fhfromfd(oldfd))) {
		errno = EBADF;
	} else if (sfd->lx_fx) {
		fd = sfd->lx_fx->lx_dup(sfd);
	} else if ((neu = __allocfd())) {
		fd = neu->lx_pos;
		*neu = *sfd;
		neu->lx_pos = fd;
	}
	__GLOBAL_STDIO_UNLOCK;
	return fd;
}
