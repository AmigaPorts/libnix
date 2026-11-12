#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include "stdio.h"

/* A copy of FD, as dup2() makes it, in the lowest free descriptor from MIN
   on.  */
static int dupfd(int fd, long min)
{
	int n, r;

	if (min < 0 || min >= SHRT_MAX) {
		errno = EINVAL;
		return -1;
	}
	__GLOBAL_STDIO_LOCK;
	for (n = min; _lx_fhfromfd(n); n++)
		;
	r = dup2(fd, n);
	__GLOBAL_STDIO_UNLOCK;
	return r;
}

/* fcntl() on an open descriptor, for libnix's fcntl() and the socket
 * library's, which sets O_NONBLOCK on a socket itself.  There is no exec,
 * so FD_CLOEXEC is accepted and does nothing.  O_APPEND is the flag write()
 * checks; O_NONBLOCK on a file is kept for F_GETFL and has no effect.  */
int __fcntl(int fd, int cmd, long arg)
{
	StdFileDes *sfd = _lx_fhfromfd(fd);

	if (!sfd) {
		errno = EBADF;
		return -1;
	}
	switch (cmd) {
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
		return dupfd(fd, arg);
	case F_GETFD:
	case F_SETFD:
		return 0;
	case F_GETFL:
		/* a socket's lx_oflags is its lx_family */
		if (sfd->lx_flags & LX_SOCKET)
			return O_RDWR | (sfd->lx_flags & LX_NONBLOCK ? O_NONBLOCK : 0);
		return sfd->lx_oflags & (O_ACCMODE | O_APPEND | O_NONBLOCK);
	case F_SETFL:
		if (sfd->lx_flags & LX_SOCKET) {
			errno = EINVAL;
			return -1;
		}
		sfd->lx_oflags = (sfd->lx_oflags & ~(O_APPEND | O_NONBLOCK))
				 | (arg & (O_APPEND | O_NONBLOCK));
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}
