#include <stdlib.h>
#include <string.h>
#include "stabs.h"
#include "amigapath.h"

/* Unix path syntax on AmigaDOS.

   "." and ".." components are always rewritten.  AmigaDOS has no such
   directories: its parent step is an empty component, "a//b" is b in the
   parent of a, so "a/./b" becomes "a/b" and "a/../b" becomes "a//b".
   Programs written for Unix produce these all the time (gcc's driver
   reaches everything through "bin/../lib"), and the rewrite cannot change
   the meaning of a native path.

   The rest is optional, enabled with the local variable NOIXPATHS set to
   "1": repeated slashes collapse to one and a leading "/vol/path" becomes
   "vol:path".  Both change the meaning of native paths, so they stay off
   unless asked for.

   The result goes into the caller's buffer (see amigapath.h), so the
   conversion is reentrant and the result outlives the call.  */

static int __pathconv;

/* Does path have a "." or ".." component?  */
static int has_dot_components(const char *p)
{
  int start = 1;

  for (; *p; p++)
  {
    if (start && p[0] == '.'
        && (p[1] == '/' || p[1] == '\0'
            || (p[1] == '.' && (p[2] == '/' || p[2] == '\0'))))
      return 1;
    start = (*p == '/' || *p == ':');
  }
  return 0;
}

/* Replace runs of '/' by a single one.  */
static void collapse_slashes(char *s)
{
  char *in = s;
  char *out = s;

  do
  {
    if (*in == '/')
      while (in[1] == '/')
        in++;
    *out++ = *in;
  } while (*in++);
}

/* Drop "." components.  */
static void remove_dot(char *s)
{
  char *in = s;
  char *out = s;
  int start = 1;

  do
  {
    while (start && in[0] == '.' && (in[1] == '/' || in[1] == '\0'))
    {
      in++;
      if (*in == '/')
        in++;
    }
    *out++ = *in;
    start = (*in == '/' || *in == ':');
  } while (*in++);
}

/* Turn ".." components into the AmigaDOS parent step: drop the dots and
   keep the slash after them, so "a/../b" reads "a//b" and a trailing ".."
   becomes "a//".  */
static void remove_dotdot(char *s)
{
  char *in = s;
  char *out = s;
  int start = 1;

  do
  {
    if (start && in[0] == '.' && in[1] == '.')
    {
      if (in[2] == '/')
        in += 2;
      else if (in[2] == '\0')
      {
        *out++ = '/';
        in += 2;
      }
    }
    *out++ = *in;
    start = (*in == '/' || *in == ':');
  } while (*in++);
}

/* "/vol/path" -> "vol:path", "/" -> "SYS:".  */
static void convert_root(char *s)
{
  char *in = s + 1;
  char *out = s;

  if (*in == '/' || *in == '\0')
  {
    strcpy(s, "SYS:");
    return;
  }
  while (*in != '/' && *in != '\0')
    *out++ = *in++;
  *out++ = ':';
  if (*in == '/')
    in++;
  do
    *out++ = *in;
  while (*in++);
}

char *__amigapath_into(const char *path, char *buf)
{
  int leading_slash;

  if (!__pathconv && !has_dot_components(path))
    return (char *)path;

  strcpy(buf, path);
  /* the Unix root reference is what the original string starts with,
     not what the ".." rewrite leaves behind ("../x" -> "/x") */
  leading_slash = (*path == '/');

  if (__pathconv)
    collapse_slashes(buf);
  remove_dot(buf);
  remove_dotdot(buf);
  if (__pathconv && leading_slash)
    convert_root(buf);

  return buf;
}

void __initamigapath(void)
{
  char *s;

  /* Check explicitly for "1", so it can be overridden locally with
     'set NOIXPATHS 0' */
  s = getenv("NOIXPATHS");
  if (s && s[0] == '1' && s[1] == '\0')
    __pathconv = 1;
}

/* Constructors run from the lowest priority up, and __fillenviron (-2)
   is what makes getenv() see the shell's variables, so this must come
   after it or NOIXPATHS is never seen.  */
ADD2INIT(__initamigapath, -1);
