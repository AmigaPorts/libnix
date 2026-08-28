#include <errno.h>
#include <string.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <sys/statvfs.h>

extern void __seterrno(void);
#ifdef IXPATHS
extern char *__amigapath(const char *path);
#endif

static unsigned long
positive_or_zero(LONG value)
{
	return value > 0 ? (unsigned long)value : 0;
}

static void
fill_statvfs(struct statvfs *buf, const struct InfoData *info)
{
	unsigned long blocks = positive_or_zero(info->id_NumBlocks);
	unsigned long used = positive_or_zero(info->id_NumBlocksUsed);
	unsigned long block_size = positive_or_zero(info->id_BytesPerBlock);
	unsigned long free_blocks;

	if (used > blocks)
		used = blocks;
	if (block_size == 0)
		block_size = 512;

	free_blocks = blocks - used;

	memset(buf, 0, sizeof(*buf));
	buf->f_bsize = block_size;
	buf->f_frsize = block_size;
	buf->f_blocks = blocks;
	buf->f_bfree = free_blocks;
	buf->f_bavail = free_blocks;
	buf->f_fsid = (unsigned long)info->id_DiskType;
	if (info->id_DiskState == ID_WRITE_PROTECTED)
		buf->f_flag |= ST_RDONLY;
}

int
statvfs(const char *name, struct statvfs *buf)
{
	struct InfoData info;
	APTR oldwin;
	APTR *wptr;
	BPTR lock;
	int ret = -1;

	if (name == NULL || buf == NULL)
	{
		errno = EFAULT;
		return -1;
	}

#ifdef IXPATHS
	if ((name = __amigapath(name)) == NULL)
		return -1;
#endif

	/* Match stat(): avoid DOS requesters while probing the volume. */
	wptr = &((struct Process *)FindTask(NULL))->pr_WindowPtr;
	oldwin = *wptr;
	*wptr = (APTR)ret;

	lock = Lock((CONST_STRPTR)name, SHARED_LOCK);
	if (lock != 0)
	{
		if (Info(lock, &info))
		{
			fill_statvfs(buf, &info);
			ret = 0;
		}
		else
			__seterrno();
		UnLock(lock);
	}
	else
		__seterrno();

	*wptr = oldwin;

	return ret;
}
