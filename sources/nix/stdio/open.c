#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <proto/dos.h>
#include "amigapath.h"

extern StdFileDes **__stdfiledes;
extern unsigned __stdfilesize;

StdFileDes *__allocfd(void);
extern void _setup_file(StdFileDes *fp);
extern void __seterrno(void);
/**
 * open a file
 *
 * @return the file descriptor.
 */
int open(const char *path, int flags, ...) {
	StdFileDes *sfd;

	if((path=__amigapath(path))==NULL)
		return -1;

	if ((sfd = __allocfd())) {
		long mode;

		sfd->lx_oflags = flags;

		mode = flags&O_TRUNC?MODE_NEWFILE: flags&O_CREAT?MODE_READWRITE:MODE_OLDFILE;
		sfd->lx_fh = (int)Open((CONST_STRPTR)path, mode);

		// workaround for RAM: and maybe other handlers which do not create a file on MODE_READWRITE.
		if (!sfd->lx_fh && (flags&O_CREAT))
			sfd->lx_fh = (int)Open((CONST_STRPTR)path, MODE_NEWFILE);
		if (sfd->lx_fh) {
			_setup_file(sfd);
			return sfd->lx_pos;
		}
		__seterrno();
		sfd->lx_inuse = 0;
	}

	return -1;
}

#ifdef __posix_threads__
	static StdFileDes *__allocfd2(void);
#endif
/**
 * Reuse or allocate a StdFileDes object.
 */
StdFileDes *__allocfd(void) {
#ifdef __posix_threads__
	__GLOBAL_STDIO_LOCK;
	StdFileDes * sfd = __allocfd2();
	__GLOBAL_STDIO_UNLOCK;
	return sfd;
}
static StdFileDes *__allocfd2(void) {
#endif
	StdFileDes *fp, **sfd;
	int file, max;

	for (sfd = __stdfiledes, max = __stdfilesize, file = 0; file < max; sfd++, file++)
		if (!sfd[0] || !sfd[0]->lx_inuse)
			break;

	if (file == SHRT_MAX) {
		errno = EMFILE;
		return NULL;
	}

	if (file == max) {
		if ((sfd = (StdFileDes **)realloc(__stdfiledes, (file + 1) * sizeof(fp))) == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		__stdfiledes = sfd;
		*(sfd = &sfd[file]) = 0;
		++__stdfilesize;
	}

	if ((fp = sfd[0]) == NULL) {
		if ((sfd[0] = fp = (StdFileDes *)malloc(sizeof(*fp))) == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		fp->lx_pos = file;
	}

	return fp;
}
