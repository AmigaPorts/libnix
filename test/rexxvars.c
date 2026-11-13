/* GetRexxVar() and SetRexxVar() of amiga.lib against a mock interpreter.
   No simulator has rexxsyslib.library, and the two functions call entries
   of it that take and return values in registers.  The mock is a jump table
   whose every slot calls one routine with d0, d1, a0 and a1; it logs the
   calls, and the test checks their order and arguments, the results, and
   the error trap SetRexxVar() sets up.  RexxVars.c is included, with
   OpenLibrary() answering for the mock.  */

#include <exec/types.h>
#include <exec/libraries.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

static struct Library *mock_open(CONST_STRPTR name);

#undef OpenLibrary
#define OpenLibrary(name, version) mock_open(name)
#undef CloseLibrary
#define CloseLibrary(base) ((void)SysBase, (void)(base))

#include "../sources/amiga/misc/RexxVars.c"

static LONG failures;

static void check(LONG ok, CONST_STRPTR what)
{
  if (!ok)
    {
      Printf("FAIL: %s\n", (LONG)what);
      failures++;
    }
}

/* ------------------------------------------------------------------ */

#define SLOTS 50

static struct { UWORD jump[SLOTS][3]; struct Library lib; } mock;
static struct RexxTask task;
static struct RexxMsg msg;
static LONG environment;
static UBYTE arena[512];
static LONG arena_used, allocations, fail_allocation, trap_in_setvalue, library_open = 1;
static struct { struct NexxStr ns; UBYTE more[8]; } stored = { { 0, 5, NSF_STRING, 0, "VALUE" } };
static UBYTE logbuf[512];
static LONG loglen;

static struct Library *mock_open(CONST_STRPTR name)
{
  return library_open && !strcmp(name, "rexxsyslib.library") ? &mock.lib : NULL;
}

static void logs(CONST_STRPTR s)
{
  while (*s && loglen < sizeof(logbuf) - 1)
    logbuf[loglen++] = *s++;
  logbuf[loglen] = 0;
}

static void logn(LONG n)
{
  UBYTE digits[12];
  LONG i = 0;

  do
    digits[i++] = '0' + n % 10;
  while ((n /= 10));
  while (i && loglen < sizeof(logbuf) - 1)
    logbuf[loglen++] = digits[--i];
  logbuf[loglen] = 0;
}

static CONST_STRPTR text(struct NexxStr *ns)
{
  return ns ? (CONST_STRPTR)ns->ns_Buff : (CONST_STRPTR)"-";
}

/* every slot of the jump table is "jsr mock_entry": the return address on
   the stack names the slot */
void mock_call(ULONG slot, struct regs *r)
{
  LONG lvo = (LONG)(slot - 6) - (LONG)&mock.lib;
  LONG i;

  if (lvo != LVO_IsRexxMsg && lvo != LVO_StrcpyN && lvo != LVO_IsSymbol && r->a0 != (lvo == LVO_CurrentEnv ? (APTR)&task : (APTR)&environment))
    logs("?a0 ");
  switch (lvo)
    {
    case LVO_IsRexxMsg:
      logs("IsRexxMsg ");
      r->d0 = r->a0 == &msg ? 0x10000 : 0;	/* a result with an empty low word */
      break;
    case LVO_CurrentEnv:
      logs("CurrentEnv ");
      r->a0 = &environment;
      break;
    case LVO_GetSpace:
      logs("GetSpace:"), logn(r->d0), logs(" ");
      if (++allocations == fail_allocation || arena_used + r->d0 > sizeof(arena))
	r->a0 = NULL;
      else
	{
	  r->a0 = arena + arena_used;
	  for (i = 0; i < r->d0; i++)
	    arena[arena_used + i] = 0xa5;
	  arena_used += (r->d0 + 1) & ~1;
	}
      r->d0 = 0xdead0001, r->d1 = 0xdead0002, r->a1 = (APTR)0xdead0003;
      break;
    case LVO_FreeSpace:
      logs("FreeSpace:"), logs(text(r->a1)), logs(":"), logn(r->d0), logs(" ");
      break;
    case LVO_StrcpyN:
      {
	UBYTE *to = r->a0, *from = r->a1, hash = 0;

	for (i = 0; i < r->d0; i++)
	  hash += to[i] = from[i];
	r->d0 = 0x5a00 | hash;
	r->a0 = r->a1 = (APTR)0xdead0004, r->d1 = 0xdead0005;
	logs("StrcpyN ");
      }
      break;
    case LVO_IsSymbol:
      {
	UBYTE *s = r->a0;

	for (i = 0; (s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == '_'; i++)
	  ;
	logs("IsSymbol:"), logs(s), logs(" ");
	r->d0 = 0xdead0006, r->d1 = 0x70000 | i, r->a0 = r->a1 = (APTR)0xdead0007;
      }
      break;
    case LVO_FetchValue:
      logs("FetchValue:"), logs(text(r->a1)), logs(":"), logs(text((struct NexxStr *)r->d0)), logs(":"), logn(r->d1), logs(" ");
      r->d1 = !strcmp(text(r->a1), "UNSET") ? 0x100 : 0;
      r->a1 = &stored.ns;
      r->d0 = 0xdead0008, r->a0 = (APTR)0xdead0009;
      break;
    case LVO_EnterSymbol:
      logs("EnterSymbol:"), logs(text(r->a1)), logs(":"), logs(text((struct NexxStr *)r->d0)), logs(" ");
      r->d0 = 0x4e0de, r->d1 = 0xdead000a, r->a0 = r->a1 = (APTR)0xdead000b;
      break;
    case LVO_SetValue:
      {
	struct NexxStr *ns = r->a1;

	logs("SetValue:"), logs(text(ns)), logs(":"), logn(ns->ns_Length), logs(":"), logn(ns->ns_Flags), logs(":"), logn(ns->ns_Hash), logs(r->d0 == 0x4e0de ? ":node " : ":? ");
	if (trap_in_setvalue)
	  {
	    /* the interpreter is out of memory: no return, no registers kept */
	    logs("trap ");
	    __asm volatile ("move.l %0,a0\n\tmove.l %1,sp\n\tsub.l a2,a2\n\tsub.l a3,a3\n\tsub.l a4,a4\n\tsub.l a5,a5\n\tsub.l a6,a6\n\tmoveq #0,d2\n\tmoveq #0,d3\n\tjmp (a0)"
			    : : "d" (task.rt_ErrTrap), "d" (task.rt_StackPtr));
	  }
	r->d0 = 0xdead000c, r->d1 = 0xdead000d, r->a0 = r->a1 = (APTR)0xdead000e;
      }
      break;
    default:
      logs("?vector:"), logn(-lvo), logs(" ");
    }
}

__asm__(
"	.text\n"
"	.even\n"
"mock_entry:\n"
"	movem.l	d0-d1/a0-a1,-(sp)\n"
"	pea	(sp)\n"
"	move.l	20(sp),-(sp)\n"
"	jsr	_mock_call\n"
"	addq.l	#8,sp\n"
"	movem.l	(sp)+,d0-d1/a0-a1\n"
"	addq.l	#4,sp\n"
"	rts\n"
);
extern void mock_entry(void) __asm("mock_entry");

static void reset(void)
{
  loglen = 0;
  logbuf[0] = 0;
  arena_used = allocations = fail_allocation = trap_in_setvalue = 0;
  task.rt_ErrTrap = (APTR)0x11111111;
  task.rt_StackPtr = (APTR)0x22222222;
}

static void expect(LONG r, LONG want, CONST_STRPTR calls, CONST_STRPTR what)
{
  Printf("%s = %ld: %s\n", (LONG)what, r, (LONG)logbuf);
  check(r == want, what);
  if (strcmp(logbuf, calls))
    {
      Printf("  expected:  %s\n", (LONG)calls);
      check(0, what);
    }
  check(task.rt_ErrTrap == (APTR)0x11111111 && task.rt_StackPtr == (APTR)0x22222222, "the task's error trap is put back");
}

int main(void)
{
  struct RexxMsg other;
  STRPTR value;
  LONG i, r;

  for (i = 0; i < SLOTS; i++)
    {
      mock.jump[i][0] = 0x4eb9;	/* jsr */
      mock.jump[i][1] = (ULONG)mock_entry >> 16;
      mock.jump[i][2] = (ULONG)mock_entry;
    }
  msg.rm_LibBase = &mock.lib;
  msg.rm_TaskBlock = &task;
  other = msg;

  reset();
  r = CheckRexxMsg(&msg);
  expect(r, 1, "IsRexxMsg ", "CheckRexxMsg");
  reset();
  expect(CheckRexxMsg(&other), 0, "IsRexxMsg ", "CheckRexxMsg, not a Rexx message");
  other.rm_LibBase = (APTR)&other;
  reset();
  expect(CheckRexxMsg(&other), 0, "", "CheckRexxMsg, another library base");
  other = msg;
  other.rm_TaskBlock = NULL;
  reset();
  expect(CheckRexxMsg(&other), 0, "", "CheckRexxMsg, no task");
  library_open = 0;
  reset();
  expect(CheckRexxMsg(&msg), 0, "", "CheckRexxMsg, no rexxsyslib.library");
  library_open = 1;

  reset();
  value = NULL;
  r = GetRexxVar(&msg, "COUNT", &value);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:14 StrcpyN IsSymbol:COUNT FetchValue:COUNT:-:0 ", "GetRexxVar");
  check(value == (STRPTR)stored.ns.ns_Buff, "GetRexxVar value");

  reset();
  value = NULL;
  r = GetRexxVar(&msg, "STEM.A.B", &value);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:17 StrcpyN GetSpace:14 StrcpyN IsSymbol:STEM. FetchValue:STEM.:STEM.A.B:0 ", "GetRexxVar, compound");
  check(value == (STRPTR)stored.ns.ns_Buff, "GetRexxVar compound value");

  reset();
  value = (STRPTR)1;
  r = GetRexxVar(&msg, "UNSET", &value);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:14 StrcpyN IsSymbol:UNSET FetchValue:UNSET:-:0 ", "GetRexxVar, no value");
  check(value == NULL, "GetRexxVar of a variable without a value gives NULL");

  reset();
  value = (STRPTR)1;
  r = GetRexxVar(&msg, "A B", &value);
  expect(r, ERR10_040, "IsRexxMsg CurrentEnv GetSpace:12 StrcpyN IsSymbol:A B FreeSpace:A B:12 ", "GetRexxVar, bad name");
  check(value == (STRPTR)1, "GetRexxVar leaves the value on an error");

  reset();
  r = GetRexxVar(&msg, "X.Y Z", &value);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:14 StrcpyN GetSpace:11 StrcpyN IsSymbol:X. FetchValue:X.:X.Y Z:0 ", "GetRexxVar, only the stem is checked");

  reset();
  fail_allocation = 1;
  r = GetRexxVar(&msg, "COUNT", &value);
  expect(r, ERR10_003, "IsRexxMsg CurrentEnv GetSpace:14 ", "GetRexxVar, no memory");

  reset();
  fail_allocation = 2;
  r = GetRexxVar(&msg, "S.T", &value);
  expect(r, ERR10_003, "IsRexxMsg CurrentEnv GetSpace:12 StrcpyN GetSpace:11 FreeSpace:S.T:12 ", "GetRexxVar, no memory for the stem");

  reset();
  other = msg;
  other.rm_LibBase = (APTR)&other;
  r = GetRexxVar(&other, "COUNT", &value);
  expect(r, ERR10_010, "", "GetRexxVar, invalid message");

  reset();
  r = SetRexxVar(&msg, "RESULT", "hello", 5);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:15 StrcpyN IsSymbol:RESULT EnterSymbol:RESULT:- GetSpace:14 StrcpyN SetValue:hello:5:2:20:node ", "SetRexxVar");

  reset();
  r = SetRexxVar(&msg, "LIST.3", "abcdef", 3);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:15 StrcpyN GetSpace:14 StrcpyN IsSymbol:LIST. EnterSymbol:LIST.:LIST.3 GetSpace:12 StrcpyN SetValue:abc:3:2:38:node ", "SetRexxVar, compound");

  reset();
  r = SetRexxVar(&msg, "RESULT", "", 0);
  expect(r, 0, "IsRexxMsg CurrentEnv GetSpace:15 StrcpyN IsSymbol:RESULT EnterSymbol:RESULT:- GetSpace:9 StrcpyN SetValue::0:2:0:node ", "SetRexxVar, empty value");

  reset();
  r = SetRexxVar(&msg, "RESULT", "x", 65536);
  expect(r, ERR10_009, "IsRexxMsg ", "SetRexxVar, value too long");

  reset();
  r = SetRexxVar(&msg, "RESULT", "x", -1);
  expect(r, ERR10_009, "IsRexxMsg ", "SetRexxVar, negative length");

  reset();
  r = SetRexxVar(&msg, "NO GOOD", "x", 1);
  expect(r, ERR10_040, "IsRexxMsg CurrentEnv GetSpace:16 StrcpyN IsSymbol:NO GOOD FreeSpace:NO GOOD:16 ", "SetRexxVar, bad name");

  reset();
  fail_allocation = 2;
  r = SetRexxVar(&msg, "RESULT", "hello", 5);
  expect(r, ERR10_003, "IsRexxMsg CurrentEnv GetSpace:15 StrcpyN IsSymbol:RESULT EnterSymbol:RESULT:- GetSpace:14 ", "SetRexxVar, no memory for the value");

  reset();
  trap_in_setvalue = 1;
  r = SetRexxVar(&msg, "RESULT", "hello", 5);
  expect(r, ERR10_003, "IsRexxMsg CurrentEnv GetSpace:15 StrcpyN IsSymbol:RESULT EnterSymbol:RESULT:- GetSpace:14 StrcpyN SetValue:hello:5:2:20:node trap ", "SetRexxVar, the interpreter traps");

  reset();
  r = SetRexxVar(&other, "RESULT", "hello", 5);
  expect(r, ERR10_010, "", "SetRexxVar, invalid message");

  if (failures)
    Printf("%ld checks failed\n", failures);
  return failures ? 20 : 0;
}
