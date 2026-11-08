#include "stdio.h"

/*
 **
 */

extern StdFileDes **__stdfiledes;
extern unsigned __stdfilesize;
extern rspinlock_t __stdLock;
extern StdFileDes *__allocfd();

int dup(int oldfd) {
	__recursiveSpinLock(&__stdLock);
	if ((unsigned)oldfd < __stdfilesize) {
		int fd;
		StdFileDes *neu, *sfd = __stdfiledes[oldfd];
		if (sfd && sfd->lx_inuse) {

			if (sfd->lx_fx) {
				int r = sfd->lx_fx->lx_dup(sfd);
				__recursiveSpinUnlock(&__stdLock);
				return r;
			}

			neu = __allocfd();
			fd = neu->lx_pos;
			*neu = *sfd;
			neu->lx_pos = fd;
			__recursiveSpinUnlock(&__stdLock);
			return fd;
		}
	}
	__recursiveSpinUnlock(&__stdLock);
	return -1;
}
