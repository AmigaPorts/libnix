#include "stdio.h"
#include <errno.h>

/*
 **
 */

extern StdFileDes **__stdfiledes;
extern unsigned __stdfilesize;

extern StdFileDes *__allocfd();

int dup(int oldfd) {
	__GLOBAL_STDIO_LOCK;
	if ((unsigned)oldfd < __stdfilesize) {
		int fd;
		StdFileDes *neu, *sfd = __stdfiledes[oldfd];
		if (sfd && sfd->lx_inuse) {

			if (sfd->lx_fx) {
				int r = sfd->lx_fx->lx_dup(sfd);
				__GLOBAL_STDIO_UNLOCK;
				return r;
			}

			neu = __allocfd();
			fd = neu->lx_pos;
			*neu = *sfd;
			neu->lx_pos = fd;
			__GLOBAL_STDIO_UNLOCK;
			return fd;
		}
	}
	__GLOBAL_STDIO_UNLOCK;
	errno = EBADF;
	return -1;
}
