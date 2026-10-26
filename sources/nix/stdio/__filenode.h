#ifndef __FILENODE_H__
#define __FILENODE_H__
/* objects in ___filelist */
#include <exec/lists.h>
struct filenode {
  struct MinNode node;
  FILE theFILE;
};

/* The size of the buffer every stream libnix opens gets: fopen()ed files,
   stdin and stdout.  __initstdio() lowers it while it sets up stdin and
   stderr. */
extern unsigned *__BUFSIZE;

/* own stuff */
extern struct MinList ___filelist; /* List of all fopen'ed files */
extern struct MinList __memorylist; /* List of memory puddles */

#endif // __FILENODE_H__
