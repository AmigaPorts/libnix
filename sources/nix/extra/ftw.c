/* Copyright 2005-2020 Rich Felker, et al.  MIT license, see nftw.c.  */

#include <ftw.h>

__stdargs int ftw(const char *path, __stdargs int (*fn)(const char *, const struct stat *, int), int fd_limit)
{
	/* The following cast assumes that calling a function with one
	 * argument more than it needs behaves as expected. This is
	 * actually undefined, but works on all real-world machines. */
	return nftw(path,
	    (__stdargs int (*)(const char *, const struct stat *, int, struct FTW *))fn,
	    fd_limit, FTW_PHYS);
}
