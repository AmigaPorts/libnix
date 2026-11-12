/* amiga.lib: the support routines of sources/amiga/misc, and a few of the
   library stubs.  Only the clib prototypes are included, so every system
   call here is a call into libamiga.a.  Each section prints what it saw and
   checks it; the expected numbers are those of the original amiga.lib.
   Without arguments the sections a simulator can run are run; an argument
   names one section.  The memory differences are printed and not checked: a
   sanitizer keeps freed blocks, and volamos's DeletePool() keeps some.  */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/commodities.h>
#include <rexx/storage.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <clib/mathieeedoubbas_protos.h>

extern struct ExecBase *SysBase;
extern ULONG RangeSeed;
VOID sprintf(STRPTR buffer, CONST_STRPTR format, ...);

/* defined here, so that the library's auto-open code stays out: CxBase and
   IconBase are mocks, and the math library is opened by hand */
struct Library *CxBase, *IconBase, *MathIeeeDoubBasBase;

/* -DAMIGALIB_ORIGINAL builds the test for a binary amiga.lib, which has
   neither of these */
#ifdef AMIGALIB_ORIGINAL
#define DeleteStdIO(io) DeleteExtIO((struct IORequest *)(io))
#define DeleteTask(task) RemTask(task)
#else
APTR AsmCreatePool(ULONG flags __asm("d0"), ULONG puddle __asm("d1"), ULONG thresh __asm("d2"), APTR sysbase __asm("a6"));
APTR AsmAllocPooled(APTR pool __asm("a0"), ULONG size __asm("d0"), APTR sysbase __asm("a6"));
VOID AsmFreePooled(APTR pool __asm("a0"), APTR mem __asm("a1"), ULONG size __asm("d0"), APTR sysbase __asm("a6"));
VOID AsmDeletePool(APTR pool __asm("a0"), APTR sysbase __asm("a6"));
#endif

static LONG failures;

static void check(LONG ok, CONST_STRPTR what)
{
  if (!ok)
    {
      Printf("FAIL: %s\n", (LONG)what);
      failures++;
    }
}

static LONG same(CONST_STRPTR a, CONST_STRPTR b)
{
  while (*a && *a == *b)
    a++, b++;
  return *a == *b;
}

/* ------------------------------------------------------------------ */

static void test_rand(void)
{
  static const ULONG fast[] = { 0x00000001, 0x80000000, 0xffffffff, 0x12345678 };
  static const ULONG fast_expect[] = { 0x00000100, 0x62be010e, 0x432bfef4, 0xcaadb1d3 };
  static const UWORD range[] = { 10, 1000, 65535, 1, 2, 0, 6, 100 };
  static const UWORD range_expect[] = { 0, 250, 16218, 0, 1, 42038, 2, 36, 0, 416, 6888, 0, 0, 34291, 5, 53 };
  ULONG seed;
  LONG i, j;

  for (i = 0; i < 4; i++)
    {
      seed = fast[i];
      for (j = 0; j < 8; j++)
	seed = FastRand(seed);
      Printf("FastRand %08lx x8 = %08lx\n", fast[i], seed);
      check(seed == fast_expect[i], "FastRand sequence");
    }

  RangeSeed = 12345;
  for (i = 0; i < 16; i++)
    {
      ULONG r = RangeRand(range[i & 7]);
      Printf("RangeRand %ld = %ld\n", (LONG)range[i & 7], r);
      check(r == range_expect[i], "RangeRand sequence");
    }
  Printf("RangeSeed %08lx\n", RangeSeed);
  check(RangeSeed == 0xa42588b1, "RangeSeed after the sequence");
}

static void test_acrypt(void)
{
  static const struct { CONST_STRPTR pw, user, expect; } t[] = {
    { "password", "user", "R^ceOXCFfS^" },
    { "", "", "WUMnBPh_INT" },
    { "a", "b", "jhsPpBL\\CBq" },
    { "secret", "guest", "eeeTbaKZtoa" },
    { "a very long password indeed", "and a long user name", "hf`[\\auMheo" },
    { "\377\376\200", "\201\377", "kcbpFXCJTdK" },
  };
  UBYTE buf[16];
  STRPTR r;
  LONG i;

  for (i = 0; i < 6; i++)
    {
      buf[12] = 0x55;
      r = ACrypt(buf, t[i].pw, t[i].user);
      Printf("ACrypt %ld = %s\n", i, (LONG)(r ? r : (STRPTR)"(null)"));
      check(r == buf && same(buf, t[i].expect), "ACrypt result");
      check(buf[12] == 0x55, "ACrypt writes 12 bytes");
    }
}

static void test_sprintf(void)
{
  UBYTE buf[80];

  sprintf(buf, "%ld %s %lx %lc|%-6ld|%06ld|%d", 123456789, "str", 0xcafeL, 'x', 42, 7, 0x12345);
  Printf("sprintf \"%s\"\n", (LONG)buf);
  check(same(buf, "123456789 str CAFE x|42    |000007|1"), "sprintf");
}

static void test_list(void)
{
  struct List l;

  l.lh_Type = 0x42;
  NewList(&l);
  Printf("NewList head %ld tail %ld tailpred %ld type %lx\n",
	 (LONG)(l.lh_Head == (struct Node *)&l.lh_Tail), (LONG)(l.lh_Tail == NULL),
	 (LONG)(l.lh_TailPred == (struct Node *)&l.lh_Head), (LONG)l.lh_Type);
  check(l.lh_Head == (struct Node *)&l.lh_Tail && !l.lh_Tail && l.lh_TailPred == (struct Node *)&l.lh_Head,
	"NewList");
  check(l.lh_Type == 0x42, "NewList leaves lh_Type");
}

static void test_port(void)
{
  static const UBYTE name[] = "libnix.amigalib.test";
  struct MsgPort *p;
  ULONG before = AvailMem(MEMF_ANY), after;
  LONG type, pri, flags, bit, task, found, empty, gone, bitfree;

  p = CreatePort(name, 5);
  check(p != NULL, "CreatePort");
  if (!p)
    return;
  type = p->mp_Node.ln_Type;
  pri = p->mp_Node.ln_Pri;
  flags = p->mp_Flags;
  bit = p->mp_SigBit;
  task = p->mp_SigTask == FindTask(NULL);
  found = FindPort(name) == p;
  empty = p->mp_MsgList.lh_TailPred == (struct Node *)&p->mp_MsgList;
  check((CONST_STRPTR)p->mp_Node.ln_Name == name, "CreatePort name");
  DeletePort(p);
  gone = FindPort(name) == NULL;
  bitfree = AllocSignal(bit) == bit;
  FreeSignal(bit);
  after = AvailMem(MEMF_ANY);
  Printf("CreatePort public: type %ld pri %ld flags %ld sigbit>=16 %ld task %ld found %ld empty %ld\n",
	 type, pri, flags, (LONG)(bit >= 16), task, found, empty);
  Printf("DeletePort: gone %ld signal free %ld memory %ld\n", gone, bitfree, before - after);
  check(type == NT_MSGPORT && pri == 5 && flags == PA_SIGNAL && bit >= 16 && task, "CreatePort fields");
  check(gone && bitfree, "DeletePort");

  p = CreatePort(NULL, -3);
  check(p != NULL, "CreatePort without a name");
  if (!p)
    return;
  type = p->mp_Node.ln_Type;
  pri = p->mp_Node.ln_Pri;
  empty = p->mp_MsgList.lh_TailPred == (struct Node *)&p->mp_MsgList
	  && p->mp_MsgList.lh_Head == (struct Node *)&p->mp_MsgList.lh_Tail;
  found = p->mp_Node.ln_Name == NULL;
  DeletePort(p);
  after = AvailMem(MEMF_ANY);
  Printf("CreatePort private: type %ld pri %ld empty %ld noname %ld memory %ld\n", type, pri, empty, found, before - after);
  check(type == NT_MSGPORT && pri == -3 && empty && found, "private port");
}

static void test_io(void)
{
  struct MsgPort *p = CreatePort(NULL, 0);
  struct IORequest *io;
  struct IOStdReq *std;
  ULONG before = AvailMem(MEMF_ANY), after;
  LONG type, len, port, clear, i;

  check(p != NULL, "CreatePort");
  if (!p)
    return;

  io = CreateExtIO(p, 100);
  check(io != NULL, "CreateExtIO");
  if (io)
    {
      type = io->io_Message.mn_Node.ln_Type;
      len = io->io_Message.mn_Length;
      port = io->io_Message.mn_ReplyPort == p;
      for (clear = 1, i = sizeof(struct Message); i < 100; i++)
	if (((UBYTE *)io)[i])
	  clear = 0;
      DeleteExtIO(io);
      after = AvailMem(MEMF_ANY);
      Printf("CreateExtIO: type %ld length %ld port %ld clear %ld memory %ld\n", type, len, port, clear, before - after);
      check(type == NT_REPLYMSG && len == 100 && port && clear, "CreateExtIO fields");
    }

  std = CreateStdIO(p);
  check(std != NULL, "CreateStdIO");
  if (std)
    {
      type = std->io_Message.mn_Node.ln_Type;
      len = std->io_Message.mn_Length;
      port = std->io_Message.mn_ReplyPort == p;
      DeleteStdIO(std);
      after = AvailMem(MEMF_ANY);
      Printf("CreateStdIO: type %ld length %ld port %ld memory %ld\n", type, len, port, before - after);
      check(type == NT_REPLYMSG && len == sizeof(struct IOStdReq) && port, "CreateStdIO fields");
    }

  io = CreateExtIO(NULL, 100);
  Printf("CreateExtIO without a port: %ld\n", (LONG)(io != NULL));
  if (io)
    DeleteExtIO(io);
  DeletePort(p);
}

static LONG pattern(UBYTE *m, LONG size, UBYTE v, LONG write)
{
  LONG i;

  for (i = 0; i < size; i++)
    if (write)
      m[i] = v;
    else if (m[i] != v)
      return 0;
  return 1;
}

static void test_pool(void)
{
  ULONG before = AvailMem(MEMF_ANY), after;
  APTR pool;
  UBYTE *a, *b, *c, *d;
  LONG clear, kept;

  pool = LibCreatePool(MEMF_CLEAR, 1024, 512);
  check(pool != NULL, "LibCreatePool");
  if (!pool)
    return;
  a = LibAllocPooled(pool, 100);
  b = LibAllocPooled(pool, 600);
  c = LibAllocPooled(pool, 16);
  check(a && b && c, "LibAllocPooled");
  if (a && b && c)
    {
      clear = pattern(a, 100, 0, 0) && pattern(b, 600, 0, 0) && pattern(c, 16, 0, 0);
      pattern(a, 100, 0x11, 1);
      pattern(b, 600, 0x22, 1);
      pattern(c, 16, 0x33, 1);
      LibFreePooled(pool, a, 100);
      d = LibAllocPooled(pool, 200);
      if (d)
	pattern(d, 200, 0x44, 1);
      kept = pattern(b, 600, 0x22, 0) && pattern(c, 16, 0x33, 0);
      LibFreePooled(pool, c, 16);
      Printf("Lib pool: clear %ld kept %ld again %ld\n", clear, kept, (LONG)(d != NULL));
      check(clear && kept && d, "Lib pool blocks");
    }
  LibDeletePool(pool);
  after = AvailMem(MEMF_ANY);
  Printf("LibDeletePool: memory %ld\n", before - after);

#ifndef AMIGALIB_ORIGINAL
  pool = AsmCreatePool(MEMF_ANY, 2048, 2048, SysBase);
  check(pool != NULL, "AsmCreatePool");
  if (!pool)
    return;
  a = AsmAllocPooled(pool, 300, SysBase);
  b = AsmAllocPooled(pool, 4000, SysBase);
  check(a && b, "AsmAllocPooled");
  if (a && b)
    {
      pattern(a, 300, 0x55, 1);
      pattern(b, 4000, 0x66, 1);
      kept = pattern(a, 300, 0x55, 0);
      AsmFreePooled(pool, a, 300, SysBase);
      check(kept && pattern(b, 4000, 0x66, 0), "Asm pool blocks");
    }
  AsmDeletePool(pool, SysBase);
  after = AvailMem(MEMF_ANY);
  Printf("AsmDeletePool: memory %ld\n", before - after);
#endif
}

/* ------------------------------------------------------------------ */

static struct Hook hook;
static LONG hookobj;

static ULONG hook_sub(struct Hook *h, APTR obj, LONG *msg)
{
  Printf("  hook %ld obj %ld data %lx msg %lx %lx %lx\n", (LONG)(h == &hook), (LONG)(obj == &hookobj),
	 (LONG)h->h_Data, msg[0], msg[1], msg[2]);
  check(h == &hook && obj == &hookobj, "hook arguments");
  return msg[0] + msg[1] + msg[2];
}

static void test_hook(void)
{
  LONG msg[3] = { 0x10, 0x200, 0x3000 };
  ULONG r;

  hook.h_Entry = (HOOKFUNC)HookEntry;
  hook.h_SubEntry = (HOOKFUNC)hook_sub;
  hook.h_Data = (APTR)0xda7a;

  r = CallHookA(&hook, (Object *)&hookobj, msg);
  Printf("CallHookA = %lx\n", r);
  check(r == 0x3210, "CallHookA");
  r = CallHook(&hook, (Object *)&hookobj, 0x1, 0x20, 0x300);
  Printf("CallHook = %lx\n", r);
  check(r == 0x321, "CallHook");
}

static struct IClass cl_a, cl_b;
static struct { struct _Object o; ULONG data; } objmem;
#define OBJ ((Object *)(&objmem.o + 1))
#define TAG_ONE (TAG_USER + 1)
#define TAG_TWO (TAG_USER + 2)

static ULONG dispatch(Class *cl, Object *o, LONG *msg)
{
  Printf("  class %s obj %ld method %lx", (LONG)(cl == &cl_a ? "A" : cl == &cl_b ? "B" : "?"), (LONG)(o == OBJ), msg[0]);
  check(o == OBJ, "dispatcher object");
  if (msg[0] == OM_SET)
    {
      struct opSet *set = (struct opSet *)msg;
      struct TagItem *t = set->ops_AttrList;

      Printf(" tags %lx=%ld %lx=%ld end %ld ginfo %ld\n", t[0].ti_Tag, t[0].ti_Data, t[1].ti_Tag, t[1].ti_Data,
	     (LONG)(t[2].ti_Tag == TAG_DONE), (LONG)(set->ops_GInfo == NULL));
      check(t[0].ti_Tag == TAG_ONE && t[0].ti_Data == 11 && t[1].ti_Tag == TAG_TWO && t[1].ti_Data == 22
	    && t[2].ti_Tag == TAG_DONE && !set->ops_GInfo, "SetSuperAttrs message");
    }
  else
    Printf(" args %ld %ld\n", msg[1], msg[2]);
  return (cl == &cl_a ? 0xa000 : 0xb000) + (msg[0] & 0xfff) + (msg[0] == OM_SET ? 0 : msg[1]);
}

static void test_method(void)
{
  LONG msg[3] = { 0x201, 7, 8 };
  ULONG r;

  cl_a.cl_Dispatcher.h_Entry = cl_b.cl_Dispatcher.h_Entry = (HOOKFUNC)HookEntry;
  cl_a.cl_Dispatcher.h_SubEntry = cl_b.cl_Dispatcher.h_SubEntry = (HOOKFUNC)dispatch;
  cl_b.cl_Super = &cl_a;
  objmem.o.o_Class = &cl_b;

  r = DoMethodA(OBJ, (Msg)msg);
  Printf("DoMethodA = %lx\n", r);
  check(r == 0xb208, "DoMethodA");
  r = DoMethod(OBJ, 0x202, 1, 2);
  Printf("DoMethod = %lx\n", r);
  check(r == 0xb203, "DoMethod");
  r = DoSuperMethodA(&cl_b, OBJ, (Msg)msg);
  Printf("DoSuperMethodA = %lx\n", r);
  check(r == 0xa208, "DoSuperMethodA");
  r = DoSuperMethod(&cl_b, OBJ, 0x203, 3, 4);
  Printf("DoSuperMethod = %lx\n", r);
  check(r == 0xa206, "DoSuperMethod");
  r = CoerceMethodA(&cl_a, OBJ, (Msg)msg);
  Printf("CoerceMethodA = %lx\n", r);
  check(r == 0xa208, "CoerceMethodA");
  r = CoerceMethod(&cl_a, OBJ, 0x204, 5, 6);
  Printf("CoerceMethod = %lx\n", r);
  check(r == 0xa209, "CoerceMethod");
  r = SetSuperAttrs(&cl_b, OBJ, TAG_ONE, 11, TAG_TWO, 22, TAG_DONE);
  Printf("SetSuperAttrs = %lx\n", r);
  check(r == 0xa103, "SetSuperAttrs");

  r = DoMethodA(NULL, (Msg)msg);
  Printf("DoMethodA without an object = %lx\n", r);
  check(r == 0, "DoMethodA without an object");
  r = DoSuperMethodA(NULL, OBJ, (Msg)msg);
  Printf("DoSuperMethodA without a class = %lx\n", r);
  check(r == 0, "DoSuperMethodA without a class");
  r = CoerceMethodA(NULL, OBJ, (Msg)msg);
  Printf("CoerceMethodA without a class = %lx\n", r);
  check(r == 0, "CoerceMethodA without a class");
}

/* ------------------------------------------------------------------ */

static void test_ievents(void)
{
  ULONG before = AvailMem(MEMF_ANY), after;
  struct InputEvent *chain = NULL, *ie;
  LONG i;

  for (i = 0; i < 3; i++)
    if ((ie = AllocMem(sizeof(*ie), MEMF_PUBLIC | MEMF_CLEAR)))
      {
	ie->ie_NextEvent = chain;
	chain = ie;
      }
  FreeIEvents(chain);
  FreeIEvents(NULL);
  after = AvailMem(MEMF_ANY);
  Printf("FreeIEvents: memory %ld\n", before - after);
}

/* commodities.library and icon.library as far as the routines call them: a
   jump table in front of a library base.  No simulator has the libraries,
   and the calls and their arguments can be checked this way.  */

struct mocklib { UWORD jump[30][3]; struct Library lib; };

static struct mocklib mock_cx, mock_icon;
static LONG mock_stray;

static void mock_other(void)
{
  mock_stray++;
}

static void mock_vector(struct mocklib *m, LONG offset, APTR function)
{
  UWORD *j = (UWORD *)((UBYTE *)&m->lib - offset);

  j[0] = 0x4ef9;	/* jmp */
  j[1] = (ULONG)function >> 16;
  j[2] = (ULONG)function;
}

static void mock_init(struct mocklib *m)
{
  LONG i;

  for (i = 1; i <= 30; i++)
    mock_vector(m, 6 * i, mock_other);
}

static UBYTE mock_description[32];

static LONG mock_ParseIX(CONST_STRPTR d __asm("a0"), IX *ix __asm("a1"))
{
  LONG n;

  for (n = 0; d[n] && n < 31; n++)
    mock_description[n] = d[n];
  mock_description[n] = 0;
  if (d[0] == '!')
    return -1;
  ix->ix_Class = IECLASS_RAWKEY;
  ix->ix_Code = 0x100 + n;
  ix->ix_Qualifier = d[0];
  return 0;
}

static LONG mock_InvertKeyMap(ULONG c __asm("d0"), struct InputEvent *ie __asm("a0"), APTR km __asm("a1"))
{
  ie->ie_Class = IECLASS_RAWKEY;
  ie->ie_Code = c;
  ie->ie_Qualifier = km ? 1 : 0;
  return TRUE;
}

static UBYTE hotkey_description[] = "alt f1";
static struct MsgPort hotkey_port;

static CxObj *mock_CreateCxObj(ULONG type __asm("d0"), LONG arg1 __asm("a0"), LONG arg2 __asm("a1"))
{
  Printf("  CreateCxObj type %ld arg1 %s arg2 %ld\n", type,
	 (LONG)(arg1 == (LONG)hotkey_description ? "description" : arg1 == (LONG)&hotkey_port ? "port" : arg1 ? "?" : "0"), arg2);
  return (CxObj *)(0x1000 + type);
}

static void mock_AttachCxObj(CxObj *head __asm("a0"), CxObj *co __asm("a1"))
{
  Printf("  AttachCxObj %lx %lx\n", (LONG)head, (LONG)co);
}

static LONG mock_CxObjError(CxObj *co __asm("a0"))
{
  Printf("  CxObjError %lx\n", (LONG)co);
  return 0;
}

static UBYTE *mock_FindToolType(CONST_STRPTR *tt __asm("a0"), CONST_STRPTR name __asm("a1"))
{
  for (; *tt; tt++)
    {
      CONST_STRPTR s = *tt, n = name;

      while (*n && *n == *s)
	n++, s++;
      if (!*n && (*s == '=' || !*s))
	return (UBYTE *)(*s ? s + 1 : s);
    }
  return NULL;
}

static LONG count(struct InputEvent *ie)
{
  LONG n;

  for (n = 0; ie; ie = ie->ie_NextEvent)
    n++;
  return n;
}

static void test_invert(void)
{
  static const UWORD expect[] = { '<', 0x106, '\t', '\r', 'B', 'a' };
  static UBYTE text[] = "aB\\n\\t<alt f1>\\<";
#ifndef AMIGALIB_ORIGINAL
  static UBYTE open_end[] = "x<alt f2";
#endif
  static UBYTE bad[] = "ab<!bad>c";
  struct InputEvent *chain, *ie;
  ULONG before, after;
  CxObj *o;
  LONG n;

  mock_init(&mock_cx);
  mock_vector(&mock_cx, 30, mock_CreateCxObj);
  mock_vector(&mock_cx, 66, mock_CxObjError);
  mock_vector(&mock_cx, 84, mock_AttachCxObj);
  mock_vector(&mock_cx, 132, mock_ParseIX);
  mock_vector(&mock_cx, 174, mock_InvertKeyMap);
  CxBase = &mock_cx.lib;

  before = AvailMem(MEMF_ANY);
  chain = InvertString(text, NULL);
  check(chain != NULL, "InvertString");
  for (n = 0, ie = chain; ie; ie = ie->ie_NextEvent, n++)
    {
      Printf("  event %ld class %ld code %lx qualifier %lx\n", n, (LONG)ie->ie_Class, (LONG)ie->ie_Code, (LONG)ie->ie_Qualifier);
      check(n < 6 && ie->ie_Class == IECLASS_RAWKEY && ie->ie_Code == expect[n], "InvertString event");
    }
  Printf("InvertString: %ld events, description \"%s\"\n", n, (LONG)mock_description);
  check(n == 6, "InvertString event count");
  check(same(mock_description, "alt f1"), "InvertString description");
  check(same(text, "aB\\n\\t<alt f1>\\<"), "InvertString leaves the string");
  FreeIEvents(chain);

  chain = InvertString("k", (APTR)&mock_cx);
  Printf("InvertString with a keymap: qualifier %ld\n", (LONG)(chain ? chain->ie_Qualifier : -1));
  check(chain && chain->ie_Qualifier == 1 && !chain->ie_NextEvent, "InvertString keymap");
  FreeIEvents(chain);

#ifndef AMIGALIB_ORIGINAL
  /* the original writes a '>' over the end of the string here */
  chain = InvertString(open_end, NULL);
  Printf("InvertString without '>': %ld events, description \"%s\"\n", count(chain), (LONG)mock_description);
  check(count(chain) == 2 && same(mock_description, "alt f2") && same(open_end, "x<alt f2"), "InvertString without '>'");
  FreeIEvents(chain);
#endif

  chain = InvertString(bad, NULL);
  Printf("InvertString bad description: %ld\n", (LONG)(chain != NULL));
  check(!chain && same(bad, "ab<!bad>c"), "InvertString bad description");
  chain = InvertString("a\\q", NULL);
  Printf("InvertString bad escape: %ld\n", (LONG)(chain != NULL));
  check(!chain, "InvertString bad escape");
  chain = InvertString("a\\", NULL);
  Printf("InvertString backslash at the end: %ld\n", (LONG)(chain != NULL));
  check(!chain, "InvertString backslash at the end");
  check(!InvertString("", NULL), "InvertString empty");
  check(!InvertString(NULL, NULL), "InvertString NULL");
  after = AvailMem(MEMF_ANY);
  Printf("InvertString: memory %ld\n", before - after);

  o = HotKey(hotkey_description, &hotkey_port, 77);
  Printf("HotKey = %lx\n", (LONG)o);
  check(o == (CxObj *)(0x1000 + CX_FILTER), "HotKey");
  Printf("other commodities calls: %ld\n", mock_stray);
  check(!mock_stray, "no other commodities calls");
}

static void test_args(void)
{
  static CONST_STRPTR argv[] = { "prog", "NAME=value", "NUM=42", "FLAG", NULL };
  CONST_STRPTR *tt;
  STRPTR s;
  LONG n;

  mock_init(&mock_icon);
  mock_vector(&mock_icon, 96, mock_FindToolType);
  IconBase = &mock_icon.lib;

  tt = (CONST_STRPTR *)ArgArrayInit(4, argv);
  Printf("ArgArrayInit CLI: %ld\n", (LONG)(tt == argv + 1));
  check(tt == argv + 1, "ArgArrayInit from the CLI");
  if (tt != argv + 1)
    return;
  s = ArgString(tt, "NAME", "default");
  Printf("ArgString NAME = %s\n", (LONG)s);
  check(same(s, "value"), "ArgString");
  s = ArgString(tt, "MISSING", "default");
  Printf("ArgString MISSING = %s\n", (LONG)s);
  check(same(s, "default"), "ArgString default");
  n = ArgInt(tt, "NUM", 7);
  Printf("ArgInt NUM = %ld\n", n);
  check(n == 42, "ArgInt");
  n = ArgInt(tt, "MISSING", 7);
  Printf("ArgInt MISSING = %ld\n", n);
  check(n == 7, "ArgInt default");
  ArgArrayDone();
  tt = (CONST_STRPTR *)ArgArrayInit(1, argv);
  Printf("ArgArrayInit without arguments: %ld\n", (LONG)(tt == NULL ? 0 : tt == argv + 1 ? 1 : 2));
  ArgArrayDone();
  Printf("other icon calls: %ld\n", mock_stray);
  check(!mock_stray, "no other icon calls");
}

static void test_delay(void)
{
  LONG r = TimeDelay(UNIT_VBLANK, 0, 40000);

  Printf("TimeDelay = %ld\n", r);
  check(r == 0, "TimeDelay");
}

/* BeginIO() with a timer request */
static void test_beginio(void)
{
  struct MsgPort *p = CreatePort(NULL, 0);
  struct timerequest *tr;

  if (!p)
    return;
  if ((tr = (struct timerequest *)CreateExtIO(p, sizeof(*tr))))
    {
      if (!OpenDevice(TIMERNAME, UNIT_VBLANK, &tr->tr_node, 0))
	{
	  tr->tr_node.io_Command = TR_ADDREQUEST;
	  tr->tr_time.tv_secs = 0;
	  tr->tr_time.tv_micro = 40000;
	  BeginIO(&tr->tr_node);
	  WaitIO(&tr->tr_node);
	  Printf("BeginIO: error %ld\n", (LONG)tr->tr_node.io_Error);
	  check(tr->tr_node.io_Error == 0, "BeginIO");
	  CloseDevice(&tr->tr_node);
	}
      else
	Printf("BeginIO: no timer.device\n");
      DeleteExtIO(&tr->tr_node);
    }
  DeletePort(p);
}

static struct Task *parent;
static ULONG child_signal;
static LONG child_ran;

/* __saveds: a new task has no a4 */
#ifdef __baserel__
#define CHILD __saveds
#else
#define CHILD
#endif
static void CHILD child(void)
{
  child_ran = 1;
  Signal(parent, child_signal);
  Wait(0);
}

static void test_task(void)
{
  static const UBYTE name[] = "libnix.amigalib.child";
  struct Task *t;
  LONG bit = AllocSignal(-1);

  parent = FindTask(NULL);
  child_signal = 1L << bit;
  t = CreateTask(name, 0, child, 4096);
  check(t != NULL, "CreateTask");
  if (t)
    {
      Printf("CreateTask: type %ld pri %ld name %ld stack %ld\n", (LONG)t->tc_Node.ln_Type, (LONG)t->tc_Node.ln_Pri,
	     (LONG)((CONST_STRPTR)t->tc_Node.ln_Name == name), (LONG)((UBYTE *)t->tc_SPUpper - (UBYTE *)t->tc_SPLower));
      Wait(child_signal);
      Printf("child ran: %ld found %ld\n", child_ran, (LONG)(FindTask(name) == t));
      check(child_ran && FindTask(name) == t, "the task runs");
      DeleteTask(t);
      Printf("DeleteTask: found %ld\n", (LONG)(FindTask(name) != NULL));
      check(!FindTask(name), "DeleteTask");
    }
  FreeSignal(bit);
}

static void test_rexx(void)
{
  static ULONG task[128];
  struct RexxMsg rm;
  STRPTR value = (STRPTR)1;
  LONG i, r;

  for (i = 0; i < sizeof(rm); i++)
    ((UBYTE *)&rm)[i] = 0;
  rm.rm_TaskBlock = (APTR)task;
  r = CheckRexxMsg(&rm);
  Printf("CheckRexxMsg on a message of no ARexx program: %ld\n", r);
  check(!r, "CheckRexxMsg");
  r = GetRexxVar(&rm, "A", &value);
  Printf("GetRexxVar: %ld, value untouched %ld\n", r, (LONG)(value == (STRPTR)1));
  check(r == 10 && value == (STRPTR)1, "GetRexxVar");
  r = SetRexxVar(&rm, "A", "1", 1);
  Printf("SetRexxVar: %ld\n", r);
  check(r == 10, "SetRexxVar");
}

/* library stubs: a WORD, a BOOL and a BYTE result, register pairs, "..." */
static void test_stubs(void)
{
  struct Task *me = FindTask(NULL);
  union { DOUBLE d; ULONG l[2]; } u;
  UBYTE buf[80];
  LONG r;

  r = SplitName("abc/def", '/', buf, 0, sizeof(buf));
  Printf("SplitName = %ld \"%s\"\n", r, (LONG)buf);
  check(r == 4 && same(buf, "abc"), "SplitName");
  r = SplitName("abc", '/', buf, 0, sizeof(buf));
  Printf("SplitName at the end = %ld\n", r);
  check(r == -1, "SplitName WORD result");

  r = Fault(ERROR_OBJECT_NOT_FOUND, "header", buf, sizeof(buf));
  Printf("Fault = %ld \"%s\"\n", (LONG)(r != 0), (LONG)buf);
  check(r != 0, "Fault");

  r = SetTaskPri(me, -3);
  Printf("SetTaskPri = %ld\n", r);
  r = SetTaskPri(me, 0);
  Printf("SetTaskPri = %ld\n", r);
  check(r == -3, "SetTaskPri BYTE result");

  r = VPrintf("VPrintf %ld %s\n", (LONG[]){ 77, (LONG)"args" });
  check(r > 0, "VPrintf");

  if ((MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library", 0)))
    {
      u.d = IEEEDPAdd(1.5, 2.25);
      Printf("IEEEDPAdd = %08lx %08lx\n", u.l[0], u.l[1]);
      check(u.l[0] == 0x400e0000 && u.l[1] == 0, "IEEEDPAdd");
      r = IEEEDPFix(-12345.0);
      Printf("IEEEDPFix = %ld\n", r);
      check(r == -12345, "IEEEDPFix");
      CloseLibrary(MathIeeeDoubBasBase);
    }
  else
    Printf("SKIP: IEEEDPAdd: no mathieeedoubbas.library\n");
}

/* ------------------------------------------------------------------ */

static const struct { CONST_STRPTR name; void (*run)(void); LONG always; } sections[] = {
  { "rand", test_rand, 1 },
  { "acrypt", test_acrypt, 1 },
  { "sprintf", test_sprintf, 1 },
  { "list", test_list, 1 },
  { "port", test_port, 1 },
  { "io", test_io, 1 },
  { "pool", test_pool, 1 },
  { "hook", test_hook, 1 },
  { "method", test_method, 1 },
  { "ievents", test_ievents, 1 },
  { "invert", test_invert, 1 },
  { "args", test_args, 1 },
  { "rexx", test_rexx, 1 },
  { "stubs", test_stubs, 1 },
  { "delay", test_delay, 1 },
  { "beginio", test_beginio, 0 },
  { "task", test_task, 0 },
};

int main(int argc, char **argv)
{
  LONG i;

  for (i = 0; i < sizeof(sections) / sizeof(sections[0]); i++)
    if (argc > 1 ? same(argv[1], sections[i].name) : sections[i].always)
      {
	Printf("== %s\n", (LONG)sections[i].name);
	sections[i].run();
      }
  if (failures)
    Printf("%ld checks failed\n", failures);
  return failures ? 20 : 0;
}
