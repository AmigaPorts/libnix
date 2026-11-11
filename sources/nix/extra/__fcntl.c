#include <errno.h>
#include <fcntl.h>
#include "stdio.h"

/* fcntl() on an open descriptor, for libnix's fcntl() and the socket
 * library's, which handles O_NONBLOCK on a socket itself.  There is no exec,
 * so FD_CLOEXEC is accepted and does nothing, and no non-blocking I/O on
 * files.  O_APPEND is the flag write() checks.  */
int __fcntl(StdFileDes *sfd, int cmd, long arg)
{
	if (!sfd) {
		errno = EBADF;
		return -1;
	}
	switch (cmd) {
	case F_GETFD:
	case F_SETFD:
		return 0;
	case F_GETFL:
		/* a socket's lx_oflags is its lx_family */
		if (sfd->lx_flags & LX_SOCKET)
			return O_RDWR;
		return sfd->lx_oflags & (O_ACCMODE | O_APPEND);
	case F_SETFL:
		if ((sfd->lx_flags & LX_SOCKET) || (arg & O_NONBLOCK)) {
			errno = EINVAL;
			return -1;
		}
		sfd->lx_oflags = (sfd->lx_oflags & ~O_APPEND) | (arg & O_APPEND);
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}
