#include <dos/dosextens.h>
#include <proto/dos.h>

#include <sys/types.h>
#include <sys/stat.h>
#include "string.h"
#include "stdio.h"

extern void __seterrno(void);

int fstat(int d, struct stat *buf) {
	extern int __stat(struct stat *buf, struct FileInfoBlock *fib);

	if (!buf)
		return -1;

	StdFileDes *sfd = _lx_fhfromfd(d);
	struct FileInfoBlock *fib;
	LONG pos, len, fh;

	if (!sfd)
		return -1;

	if (sfd->lx_fx)
		return sfd->lx_fx->lx_fstat(sfd, buf);

	if (!(fh = sfd->lx_fh))
		return -1;

	memset(buf, 0, sizeof(*buf));

	buf->st_mode = S_IFCHR | 0777;
	buf->st_nlink = 1;
	buf->st_blksize = 512;
	/*buf->st_blocks  = 0;*/

	/* NIL: has no handler, and a console is a character device.
	   IsInteractive() reads a flag in the handle, so this costs no packet:
	   stdio asks this of fds 0-2.  */
	if (!((struct FileHandle*) BADDR(fh))->fh_Type || IsInteractive(fh))
		return 0;

	if ((fib = (struct FileInfoBlock*) AllocDosObject(DOS_FIB, NULL)) == NULL) {
		__seterrno();
		return -1;
	}

	if (!ExamineFH(fh, fib)) {
		/* No ExamineFH(): a file if it seeks, and its size is the end.
		   Pipes and the other stream handlers know neither, and their
		   Seek() returns 0 rather than -1, with IoErr() set to
		   ERROR_ACTION_NOT_KNOWN: 0 is also the position of a file just
		   opened, so only IoErr() tells them apart.  */
		SetIoErr(0);
		pos = Seek(fh, 0, OFFSET_END);
		if (pos == -1 || IoErr() == ERROR_ACTION_NOT_KNOWN) {
			FreeDosObject(DOS_FIB, fib);
			return 0;
		}
		/* Seeking back can only fail on a broken handler, but then the
		   position has moved.  */
		if ((len = Seek(fh, pos, OFFSET_BEGINNING)) == -1) {
			__seterrno();
			FreeDosObject(DOS_FIB, fib);
			return -1;
		}

		fib->fib_DiskKey = (ino_t) ~((LONG) fh);
		fib->fib_DirEntryType = -1;
		fib->fib_Size = len;
		fib->fib_Protection = 0xff0;
		fib->fib_NumBlocks = (len + 511) >> 9;
		DateStamp(&fib->fib_Date);
	}

	__stat(buf, fib);

	FreeDosObject(DOS_FIB, fib);
	return 0;
}
