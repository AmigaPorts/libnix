#if 1

#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>

struct newMemList {
  struct Node     nml_Node;
  UWORD           nml_NumEntries;
  struct MemEntry nml_ME[2];
};

static const struct newMemList MemTemplate = {
  {0,},
  2,
  { {MEMF_CLEAR|MEMF_PUBLIC, sizeof(struct Task)},
    {MEMF_CLEAR, 0} }
};

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    /*(l)->lh_Tail = NULL,*/ \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

struct Task *CreateTask(STRPTR name, LONG pri, APTR initpc, ULONG stacksize)
{ struct Library *SysBase = *(struct Library **)4L;
  struct newMemList nml;
  struct MemList *ml;
  struct Task *newtask;
  APTR task2;

  stacksize=(stacksize+3)&~3;

  { long *p1,*p2;
    int i;

    for (p1=(long *)&nml,p2=(long*)&MemTemplate,i=7; i; *p1++=*p2++,i--) ;
    *p1=stacksize;
  }

  ml=AllocEntry((struct MemList *)&nml);
  if (!(((unsigned int)ml) & (1<<31))) {
    newtask=ml->ml_ME[0].me_Addr;
    newtask->tc_Node.ln_Type = NT_TASK;
    newtask->tc_Node.ln_Pri  = pri;
    newtask->tc_Node.ln_Name = name;
    newtask->tc_SPReg        = (APTR)((ULONG)ml->ml_ME[1].me_Addr+stacksize);
    newtask->tc_SPLower      = ml->ml_ME[1].me_Addr;
    newtask->tc_SPUpper      = newtask->tc_SPReg;
    NEWLIST(&newtask->tc_MemEntry);
    AddHead(&newtask->tc_MemEntry,&ml->ml_Node);
    task2=AddTask(newtask,initpc,0);
    if (SysBase->lib_Version>36 && !task2) {
      FreeEntry(ml); newtask = NULL;
    }
  }
  else
    newtask = NULL;

  return newtask;
}

#else

asm(
"		.globl	_CreateTask\n"

"_CreateTask:	moveml	d2/d3/a2/a3/a6,sp@-\n"
"		movel	sp@(12+6*4:W),d0\n"
"		addql	#3,d0\n"
"		moveq	#-4,d2\n"
"		andl	d0,d2			| stack adjusted\n"
"		movel	d2,sp@-\n"
"		lea	pc@(Lmemlist-.+2+7*4),a0\n"
"		moveq	#6,d0\n"
"L3:		movel	a0@-,sp@-		| copy memlist\n"
"		dbra	d0,L3\n"
"		movel	sp,a0\n"
"		movel	4:W,a6\n"
"		jsr	a6@(-222:W)		| AllocEntry()\n"
"		movel	d0,a2\n"
"		movel	a2,d0\n"
"		lea	sp@(32:W),sp\n"
"		blts	L2\n"
"		movel	a2@(16:W),a3\n"
"		moveb	#1,a3@(8:W)		| ln_Type\n"
"		moveb	sp@(7+6*4:W),a3@(9:W)	| ln_Pri\n"
"		movel	sp@(0+6*4:W),a3@(10:W)	| ln_Name\n"
"		movel	a2@(24:W),d0\n"
"		addl	d0,d2\n"
"		movel	d2,a3@(54:W)		| tc_SPReg\n"
"		movel	d0,a3@(58:W)		| tc_SPLower\n"
"		movel	d2,a3@(62:W)		| tc_SPUpper\n"
"		lea	a3@(74:W),a0		| tc_MemList\n"
"		movel	a0,a0@(8:W)\n"
"		addqw	#4,a0\n"
"		movel	a0,a0@-\n"
"		movel	a2,a1\n"
"		jsr	a6@(-240:W)		| AddHead()\n"
"		movel	a3,d3\n"
"		movel	d3,a1\n"
"		movel	sp@(8+6*4:W),d2\n"
"		exg	d2,a2\n"
"		subal	a3,a3\n"
"		jsr	a6@(-282:W)		| AddTask()\n"
"		cmpw	#37,a6@(20:W)\n"
"		bcss	L1			| kick2.0+ ?\n"
"		tstl	d0\n"
"		bnes	L1			| task added\n"
"		movel	d2,a0\n"
"		jsr	a6@(-228:W)		| FreeEntry()\n"
"L2:		moveq	#0,d3\n"
"L1:		movel	d3,d0			| new task\n"
"		moveml	sp@+,d2/d3/a2/a3/a6\n"
"		rts\n"

"		.align	2\n"

"Lmemlist:	.long	0,0			| Succ,Pred\n"
"		.byte	0,0			| Pri,Type\n"
"		.long	0			| Name\n"
"		.word	2			| NumEntries\n"
"		.long	0x10001			| MemType\n"
"		.long	92			| Length\n"
"		.long	0x10000			| MemType\n"
"|		.long	0			| Length\n"
);

#endif
