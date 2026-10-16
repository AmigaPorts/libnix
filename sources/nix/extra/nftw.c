/*
 * Copyright 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* From musl, by way of newlib.  AmigaDOS changes:
   - no separator after a volume or assign name: "DH0:x", since "DH0:/x"
     is in the parent;
   - lstat() follows soft links, so FTW_PHYS relies on readdir()'s d_type
     instead.  A soft link given as the starting path is still followed;
   - stat() leaves st_dev 0, so FTW_MOUNT compares the locks' volumes;
   - no cycle check against a zero inode, which some handlers report for
     every file;
   - fd_limit does not limit the depth: directories cost a lock, not a
     file descriptor, so the whole tree is visited.  */

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "amigapath.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef __stdargs int (*nftw_fn)(const char *, const struct stat *, int, struct FTW *);

struct history
{
	struct history *chain;
	BPTR volume;
	ino_t ino;
	int level;
	int base;
};

/* The volume path is on, or 0.  */
static BPTR volume_of(const char *path)
{
	const char *apath = __amigapath(path);
	BPTR lock, volume = 0;

	if (apath && (lock = Lock((CONST_STRPTR)apath, SHARED_LOCK)) != 0) {
		volume = ((struct FileLock *)BADDR(lock))->fl_Volume;
		UnLock(lock);
	}
	return volume;
}

/* dtype is the entry's d_type, DT_UNKNOWN for the starting path.  */
static int do_nftw(char *path, int dtype, nftw_fn fn, int flags, struct history *h)
{
	size_t l = strlen(path), j = l && path[l-1]=='/' ? l-1 : l;
	/* Length of the separator between path and a directory entry: none
	   right after a volume name, but "SDK:/" is SDK:'s parent.  */
	size_t sep = l && path[l-1]==':' ? 0 : 1;
	struct stat st;
	struct history new;
	int type;
	int r;
	struct FTW lev;

	memset(&st, 0, sizeof(st));
	if ((flags & FTW_PHYS) && dtype == DT_LNK) {
		st.st_mode = S_IFLNK | 0777;
		type = FTW_SL;
	} else if (stat(path, &st) < 0) {
		if (!(flags & FTW_PHYS) && errno==ENOENT && dtype == DT_LNK)
			type = FTW_SLN;
		else if (errno != EACCES) return -1;
		else type = FTW_NS;
	} else if (S_ISDIR(st.st_mode)) {
		if (access(path, R_OK) < 0) type = FTW_DNR;
		else if (flags & FTW_DEPTH) type = FTW_DP;
		else type = FTW_D;
	} else {
		type = FTW_F;
	}

	new.volume = (flags & FTW_MOUNT) ? volume_of(path) : 0;
	if ((flags & FTW_MOUNT) && h && new.volume != h->volume)
		return 0;

	new.chain = h;
	new.ino = st.st_ino;
	new.level = h ? h->level+1 : 0;
	new.base = j+sep;

	lev.level = new.level;
	if (h) {
		lev.base = h->base;
	} else {
		size_t k;
		for (k=j; k && path[k]=='/'; k--);
		for (; k && path[k-1]!='/' && path[k-1]!=':'; k--);
		lev.base = k;
	}

	if (!(flags & FTW_DEPTH) && (r=fn(path, &st, type, &lev)))
		return r;

	if (st.st_ino)
		for (; h; h = h->chain)
			if (h->ino == st.st_ino)
				return 0;

	if (type == FTW_D || type == FTW_DP) {
		DIR *d = opendir(path);
		if (d) {
			struct dirent *de;
			while ((de = readdir(d))) {
				if (de->d_name[0] == '.'
				 && (!de->d_name[1]
				  || (de->d_name[1]=='.'
				   && !de->d_name[2]))) continue;
				if (strlen(de->d_name) >= PATH_MAX-l) {
					errno = ENAMETOOLONG;
					closedir(d);
					return -1;
				}
				if (sep)
					path[j]='/';
				strcpy(path+j+sep, de->d_name);
				if ((r=do_nftw(path, de->d_type, fn, flags, &new))) {
					closedir(d);
					return r;
				}
			}
			closedir(d);
		} else if (errno != EACCES) {
			return -1;
		}
	}

	path[l] = 0;
	if ((flags & FTW_DEPTH) && (r=fn(path, &st, type, &lev)))
		return r;

	return 0;
}

__stdargs int nftw(const char *path, nftw_fn fn, int fd_limit, int flags)
{
	size_t l;
	char pathbuf[PATH_MAX+1];

	if (fd_limit <= 0) return 0;

	l = strlen(path);
	if (l > PATH_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(pathbuf, path, l+1);

	return do_nftw(pathbuf, DT_UNKNOWN, fn, flags, NULL);
}
