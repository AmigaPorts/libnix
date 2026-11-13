#include <exec/libraries.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <rexx/errors.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

/* CheckRexxMsg(), GetRexxVar() and SetRexxVar() of amiga.lib: the variables
   of the ARexx program that sent a message.

   Only IsRexxMsg() is a documented function of rexxsyslib.library.  The
   others are entries of the interpreter that the include files do not name;
   they take and return values in d0, d1, a0 and a1, as noted at each call. */

#define LVO_EnterSymbol -66
#define LVO_FetchValue  -72
#define LVO_SetValue    -84
#define LVO_IsSymbol    -102
#define LVO_CurrentEnv  -108
#define LVO_GetSpace    -114
#define LVO_FreeSpace   -120
#define LVO_IsRexxMsg   -168
#define LVO_StrcpyN     -270

struct regs { ULONG d0, d1; APTR a0, a1; };

/* call one entry of the library with these registers, and return them */
static void rx_call(struct Library *base, LONG lvo, struct regs *r)
{
  register ULONG d0 __asm("d0") = r->d0;
  register ULONG d1 __asm("d1") = r->d1;
  register APTR a0 __asm("a0") = r->a0;
  register APTR a1 __asm("a1") = r->a1;
  register APTR entry __asm("a2") = (UBYTE *)base+lvo;
  register struct Library *a6 __asm("a6") = base;

  __asm volatile ("jsr (%4)"
		  : "+r" (d0), "+r" (d1), "+r" (a0), "+r" (a1)
		  : "a" (entry), "r" (a6)
		  : "cc", "memory");
  r->d0 = d0; r->d1 = d1; r->a0 = a0; r->a1 = a1;
}

/* Without memory the interpreter does not return to its caller: it jumps to
   rt_ErrTrap with the stack pointer set to rt_StackPtr.  rx_guard() calls
   function(argument) with both pointing at itself and puts the task's own
   values back; the result is the function's, or ERR10_003 from the trap.
   No register survives the trap, which is why this is not C.
   trap points at rt_ErrTrap; rt_StackPtr follows it. */
LONG rx_guard(APTR *trap, LONG (*function)(APTR), APTR argument) __asm("rx_guard");

__asm__(
"	.text\n"
"	.even\n"
"rx_guard:\n"
"	movem.l	d2-d7/a2-a6,-(sp)\n"
"	move.l	48(sp),a0\n"
"	move.l	4(a0),-(sp)\n"
"	move.l	(a0),-(sp)\n"
"	move.l	a0,-(sp)\n"
"	lea	rx_trapped(pc),a1\n"
"	move.l	a1,(a0)\n"
"	move.l	sp,4(a0)\n"
"	move.l	68(sp),-(sp)\n"
"	move.l	68(sp),a0\n"
"	jsr	(a0)\n"
"	addq.l	#4,sp\n"
"	jra	rx_unguard\n"
"rx_trapped:\n"
"	moveq	#3,d0\n"		/* ERR10_003 */
"rx_unguard:\n"
"	move.l	(sp)+,a0\n"
"	move.l	(sp)+,(a0)\n"
"	move.l	(sp)+,4(a0)\n"
"	movem.l	(sp)+,d2-d7/a2-a6\n"
"	rts\n"
);

struct rexx {
  struct Library *base;
  APTR env;			/* the program's current environment */
};

/* a string of the interpreter with this text, NULL without memory */
static struct NexxStr *rx_string(struct rexx *rx, CONST_STRPTR text, LONG length)
{
  /* a0 environment, d0 size: a0 block */
  struct regs space = { .a0 = rx->env, .d0 = length+NXADDLEN };
  struct NexxStr *ns;

  rx_call(rx->base,LVO_GetSpace,&space);
  if ((ns=space.a0)) {
    /* a0 to, a1 from, d0 length: d0 hash */
    struct regs copy = { .a0 = ns->ns_Buff, .a1 = (APTR)text, .d0 = length };

    ns->ns_Ivalue = 0;
    ns->ns_Length = length;
    ns->ns_Flags  = NSF_STRING;
    ns->ns_Buff[length] = 0;
    rx_call(rx->base,LVO_StrcpyN,&copy);
    ns->ns_Hash = copy.d0;
  }
  return ns;
}

static void rx_free(struct rexx *rx, struct NexxStr *ns)
{
  if (ns && !(ns->ns_Flags&NSF_KEEP)) {
    /* a0 environment, a1 block, d0 size */
    struct regs space = { .a0 = rx->env, .a1 = ns, .d0 = ns->ns_Length+NXADDLEN };

    rx_call(rx->base,LVO_FreeSpace,&space);
  }
}

/* The strings that name a variable: the symbol itself, or for a compound
   symbol its stem, up to the first dot, and the whole name. */
struct symbol { struct NexxStr *stem, *whole; };

static LONG rx_symbol(struct rexx *rx, CONST_STRPTR name, struct symbol *sym)
{
  LONG length,stem,err = ERR10_003;

  for (length=0; name[length]; length++)
    ;
  for (stem=0; stem<length && name[stem]!='.'; stem++)
    ;

  sym->whole = NULL;
  if (stem<length) {
    if (!(sym->whole=rx_string(rx,name,length)))
      return err;
    length = stem+1;
  }
  if ((sym->stem=rx_string(rx,name,length))) {
    /* a0 text: d1 length of the symbol it starts with */
    struct regs symbol = { .a0 = sym->stem->ns_Buff };

    rx_call(rx->base,LVO_IsSymbol,&symbol);
    if ((UWORD)symbol.d1==sym->stem->ns_Length)
      return 0;
    err = ERR10_040;
    rx_free(rx,sym->stem);
  }
  rx_free(rx,sym->whole);
  return err;
}

/* the message's interpreter and the environment of its program */
static void rx_open(struct rexx *rx, CONST struct RexxMsg *msg)
{
  /* a0 task: a0 environment */
  struct regs env = { .a0 = msg->rm_TaskBlock };

  rx->base = msg->rm_LibBase;
  rx_call(rx->base,LVO_CurrentEnv,&env);
  rx->env = env.a0;
}

BOOL CheckRexxMsg(CONST struct RexxMsg *msg)
{ APTR SysBase = *(APTR *)4L;
  struct regs is = { .a0 = (APTR)msg };
  struct Library *base;

  /* the sender keeps the library open while its message is out */
  if (!(base=OpenLibrary(RXSNAME,0)))
    return FALSE;
  CloseLibrary(base);

  if (msg->rm_LibBase!=base || !msg->rm_TaskBlock)
    return FALSE;
  rx_call(base,LVO_IsRexxMsg,&is);
  return is.d0!=0;
}

LONG GetRexxVar(CONST struct RexxMsg *msg, CONST_STRPTR name, STRPTR *value)
{
  struct symbol sym;
  struct rexx rx;
  LONG err;

  if (!CheckRexxMsg(msg))
    return ERR10_010;
  rx_open(&rx,msg);
  if (!(err=rx_symbol(&rx,name,&sym))) {
    /* a0 environment, a1 stem, d0 whole name, d1 0:
       a1 value, d1 set if the variable has none */
    struct regs fetch = { .a0 = rx.env, .a1 = sym.stem, .d0 = (ULONG)sym.whole };

    rx_call(rx.base,LVO_FetchValue,&fetch);
    *value = fetch.d1 ? NULL : (STRPTR)((struct NexxStr *)fetch.a1)->ns_Buff;
  }
  return err;
}

struct assignment {
  CONST struct RexxMsg *msg;
  CONST_STRPTR name,value;
  LONG length;
};

static LONG rx_assign(APTR argument)
{
  struct assignment *as = argument;
  struct NexxStr *value;
  struct symbol sym;
  struct rexx rx;
  LONG err;

  rx_open(&rx,as->msg);
  if (!(err=rx_symbol(&rx,as->name,&sym))) {
    /* a0 environment, a1 stem, d0 whole name: d0 node */
    struct regs enter = { .a0 = rx.env, .a1 = sym.stem, .d0 = (ULONG)sym.whole };

    rx_call(rx.base,LVO_EnterSymbol,&enter);
    if ((value=rx_string(&rx,as->value,as->length))) {
      /* a0 environment, a1 value, d0 node */
      struct regs set = { .a0 = rx.env, .a1 = value, .d0 = enter.d0 };

      rx_call(rx.base,LVO_SetValue,&set);
    } else
      err = ERR10_003;
  }
  return err;
}

LONG SetRexxVar(struct RexxMsg *msg, CONST_STRPTR name, CONST_STRPTR value, LONG length)
{
  struct assignment as = { msg, name, value, length };

  if (!CheckRexxMsg(msg))
    return ERR10_010;
  if (length<0 || length>65535)
    return ERR10_009;
  return rx_guard(&((struct RexxTask *)msg->rm_TaskBlock)->rt_ErrTrap,rx_assign,&as);
}
