#include <stdlib.h>
#include <errno.h>
#ifdef __posix_threads__
#include <proto/exec.h>
#endif
#include "stabs.h"
#include "stdio.h"

/* One list for atexit() and __cxa_atexit(), so exit() calls the functions
   in the reverse order of their registration whichever way they were
   registered, as C and C++ require. A function registered while exit() runs
   the list (a static object first used in a destructor) is called too. */
struct atexitnode
{ 
  struct atexitnode *next;
  void (*func)(void);        /* from atexit() */
  void (*cxafunc)(void *);   /* from __cxa_atexit() */
  void *arg;
};

static struct atexitnode *__funclist=NULL; /* List of functions to call at exit */

static int __addexit(void (*func)(void), void (*cxafunc)(void *), void *arg)
{ 
  struct atexitnode *node;
  if((node=(struct atexitnode *)malloc(sizeof(struct atexitnode)))==NULL)
  { errno=ENOMEM;
    return -1; }
  node->func=func;
  node->cxafunc=cxafunc;
  node->arg=arg;
#ifdef __posix_threads__
  Forbid();
#endif
  node->next=__funclist;
  __funclist=node;
#ifdef __posix_threads__
  Permit();
#endif
  return 0;
}

int atexit(void (*func)(void))
{ return __addexit(func,NULL,NULL); }

/* A program is one image, so dso needs no bookkeeping.  */
int __cxa_atexit(void (*func)(void *), void *arg, void *dso)
{ (void)dso;
  return __addexit(NULL,func,arg); }

/* Unlink each function before calling it: a callback may call exit() or
   register another callback.  Use the registration lock for removal too.  */
void __exitatexit(void)
{
  struct atexitnode *thisf;
  for (;;) {
#ifdef __posix_threads__
    Forbid();
#endif
    thisf=__funclist;
    if (thisf)
      __funclist=thisf->next;
#ifdef __posix_threads__
    Permit();
#endif
    if (!thisf)
      break;
    if(thisf->cxafunc!=NULL)
      (*thisf->cxafunc)(thisf->arg);
    else
      (*thisf->func)();
  }
}

ADD2EXIT(__exitatexit,0);
