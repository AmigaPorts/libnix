/* The installed libamiga.a, as the build wrote it from the SFD files: library
   stubs, with arguments and results narrower than a long, a varargs stub and
   the LVO symbols.  Only the clib prototypes are included, so every system
   call here is a call into libamiga.a.  The test
   defines no library base of its own: the stubs take them from libstubs.a,
   and the startup code opens them.  */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <graphics/gfxbase.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>

extern struct Library *UtilityBase;
extern struct GfxBase *GfxBase;

static LONG failures;

static void check(LONG ok, CONST_STRPTR what)
{
  if (!ok)
    {
      Printf("FAIL: %s\n", (LONG)what);
      failures++;
    }
}

/* The LVO symbols are absolute: assembler sources use them as offsets.  */
static LONG lvo_sum(void)
{
  LONG sum;

  __asm__("move.l #_LVOOpen,%0\n\tadd.l #_LVOAllocMem,%0\n\tadd.l #_LVOFindTagItem,%0" : "=d" (sum));
  return sum;
}

int main(void)
{
  struct TagItem tags[] = { { TAG_USER + 1, 11 }, { TAG_USER + 2, 22 }, { TAG_DONE, 0 } };
  struct FileInfoBlock *fib;
  struct Task *me;
  UBYTE part[8];
  APTR mem;
  BYTE pri;
  WORD pos;

  check(lvo_sum() == -30 - 198 - 30, "LVO symbols");

  mem = AllocMem(64, MEMF_PUBLIC | MEMF_CLEAR);
  check(mem != NULL, "AllocMem");
  if (mem)
    FreeMem(mem, 64);

  check(UtilityBase != NULL && UtilityBase != (struct Library *)-1, "UtilityBase is open");
  check(FindTagItem(TAG_USER + 2, tags) == &tags[1], "FindTagItem");
  check(GetTagData(TAG_USER + 1, 0, tags) == 11, "GetTagData");
  check(UDivMod32(100, 7) == 14, "UDivMod32");
  check(SMult32(-3, 7) == -21, "SMult32");

  /* a UBYTE above 127, in and out: Latin-1 e acute */
  check(ToUpper('a') == 'A', "ToUpper('a')");
  check(ToUpper(0xe9) == 0xc9, "ToUpper(0xe9)");

  /* a negative BYTE, in and out */
  me = FindTask(NULL);
  pri = SetTaskPri(me, -5);
  check(me->tc_Node.ln_Pri == -5, "SetTaskPri(-5) sets -5");
  check(SetTaskPri(me, pri) == -5, "SetTaskPri returns -5");
  check(me->tc_Node.ln_Pri == pri, "SetTaskPri restores the priority");

  /* a UBYTE separator and a WORD position, -1 at the last part */
  pos = SplitName((CONST_STRPTR)"ab/cd", '/', part, 0, sizeof part);
  check(pos == 3 && part[0] == 'a' && part[1] == 'b' && !part[2], "SplitName first part");
  pos = SplitName((CONST_STRPTR)"ab/cd", '/', part, pos, sizeof part);
  check(pos == -1 && part[0] == 'c' && part[1] == 'd' && !part[2], "SplitName last part");

  check(GfxBase != NULL && GfxBase != (struct GfxBase *)-1, "GfxBase is open");

  fib = AllocDosObjectTags(DOS_FIB, TAG_DONE);
  check(fib != NULL, "AllocDosObjectTags");
  if (fib)
    FreeDosObject(DOS_FIB, fib);

  SetIoErr(1234);
  check(IoErr() == 1234, "SetIoErr and IoErr");

  return failures ? 20 : 0;
}
