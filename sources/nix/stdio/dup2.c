#include "stdio.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

extern StdFileDes **__stdfiledes;
extern unsigned __stdfilesize;

/* Make newfd a copy of oldfd, closing newfd first if it is open.  A copy
   shares the original's handle, which close() keeps open until the last of
   them is closed; a socket gets a socket of its own from its dup hook.  */
int dup2(int oldfd, int newfd) {
	StdFileDes *old, *neu;
	int r = -1;

	__GLOBAL_STDIO_LOCK;
	if (!(old = _lx_fhfromfd(oldfd)) || (unsigned) newfd >= SHRT_MAX) {
		errno = EBADF;
		goto out;
	}
	if (newfd == oldfd) {
		r = newfd;
		goto out;
	}
	if ((unsigned) newfd >= __stdfilesize) {
		StdFileDes **sfd = realloc(__stdfiledes, ((unsigned) newfd + 1) * sizeof(*sfd));
		if (!sfd) {
			errno = ENOMEM;
			goto out;
		}
		__stdfiledes = sfd;
		while (__stdfilesize <= (unsigned) newfd)
			sfd[__stdfilesize++] = NULL;
	}
	close(newfd);

	if (old->lx_fx) {
		/* the new socket is in the lowest free slot: swap it into newfd's */
		int fd = old->lx_fx->lx_dup(old);
		if (fd >= 0) {
			neu = __stdfiledes[newfd];
			if ((__stdfiledes[newfd] = __stdfiledes[fd]))
				__stdfiledes[newfd]->lx_pos = newfd;
			if ((__stdfiledes[fd] = neu))
				neu->lx_pos = fd;
			r = newfd;
		}
		goto out;
	}

	if (!(neu = __stdfiledes[newfd]) && !(neu = __stdfiledes[newfd] = malloc(sizeof(*neu)))) {
		errno = ENOMEM;
		goto out;
	}
	*neu = *old;
	neu->lx_pos = r = newfd;
out:
	__GLOBAL_STDIO_UNLOCK;
	return r;
}
