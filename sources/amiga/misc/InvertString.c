#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/keymap.h>
#include <libraries/commodities.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/commodities.h>

/* InvertString() of amiga.lib: the input events that type str, the last
   character first, as AddIEvents() takes them.  \n and \r are return, \t is
   tab, \0 is NUL, and \\ \" \' \< stand for themselves; <description> is an
   input description for ParseIX().  NULL if str is empty or wrong, or
   without memory. */

/* the character a backslash escapes, -1 if none */
static LONG escape(UBYTE c)
{
  switch (c) {
    case 'n':
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case '0':
      return 0;
    case '\\':
    case '"':
    case '\'':
    case '<':
      return c;
  }
  return -1;
}

/* *strp is behind a '<': fill ie from the description and leave *strp on
   its '>'.  The '>' is a NUL while ParseIX() reads, as in the original, so
   str must be writable.  Without a '>' the description is the rest of str. */
static BOOL description(STRPTR *strp, struct InputEvent *ie)
{
  STRPTR end = *strp;
  LONG err;
  IX ix;

  while (*end && *end!='>')
    end++;
  if (*end) {
    *end = 0;
    err = ParseIX(*strp,&ix);
    *end = '>';
    *strp = end;
  } else {
    err = ParseIX(*strp,&ix);
    *strp = end-1;
  }
  if (err)
    return FALSE;

  ie->ie_Class     = ix.ix_Class;
  ie->ie_Code      = ix.ix_Code;
  ie->ie_Qualifier = ix.ix_Qualifier;
  return TRUE;
}

struct InputEvent *InvertString(CONST_STRPTR str, CONST struct KeyMap *km)
{ APTR SysBase = *(APTR *)4L;
  struct InputEvent *chain = NULL, *ie;
  STRPTR s = (STRPTR)str;
  LONG c;

  if (!s || !*s)
    return NULL;

  do {
    if (!(ie=AllocMem(sizeof(*ie),MEMF_CLEAR|MEMF_PUBLIC)))
      goto fail;
    ie->ie_NextEvent = chain;
    chain = ie;

    c = *s;
    if (c=='<') {
      s++;
      if (!description(&s,ie))
        goto fail;
    } else {
      if (c=='\\' && (c=escape(*++s))<0)
        goto fail;
      InvertKeyMap(c,ie,km);
    }
  } while (*++s);

  return chain;

fail:
  FreeIEvents(chain);
  return NULL;
}
