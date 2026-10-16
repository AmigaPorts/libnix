#include <stdlib.h>
#include <errno.h>
#include "stabs.h"
#include "stdio.h"

struct atexitnode
{ 
  struct atexitnode *next;
  void (*func)(void);
};

static struct atexitnode *__funclist=NULL; /* List of functions to call at exit */

int atexit(void (*func)(void))
{ 
  struct atexitnode *node;
  if((node=(struct atexitnode *)malloc(sizeof(struct atexitnode)))==NULL)
  { errno=ENOMEM;
    return -1; }
  else
  { node->next=__funclist;
    node->func=func;
    __funclist=node;
    return 0; }
}

/* Unlink each function before calling it: if it calls exit(), the exit
   list runs again and must go on with the next one, not loop forever.  */
void __exitatexit(void)
{
  struct atexitnode *thisf;
  while((thisf=__funclist)!=NULL)
  { __funclist=thisf->next;
    (*thisf->func)(); }
}

ADD2EXIT(__exitatexit,0);
