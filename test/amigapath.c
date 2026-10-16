/* Table test for the Unix-to-AmigaDOS path rewrite in
   sources/nix/extra/__amigapath.c, built and run on the build machine
   with the host compiler: the rewrite is plain string handling and needs
   nothing from AmigaOS.  The constructor macro is stubbed out by the local
   stabs.h.  Run with "make -C test" or through .github/workflows/test.yml.  */

#include <stdio.h>
#include <string.h>

#include "../sources/nix/extra/__amigapath.c"

static int fails;

static void check(const char *in, const char *want)
{
  char buf[256];
  const char *got = __amigapath_into(in, buf);
  int ok = got && strcmp(got, want) == 0;

  printf("%s %-60s -> %s\n", ok ? "ok  " : "FAIL", in, got ? got : "(NULL)");
  if (!ok)
    {
      printf("     want %s\n", want);
      fails++;
    }
}

/* a path without dot components must come back as the same pointer,
   without a copy */
static void check_untouched(const char *in)
{
  char buf[256];
  const char *got = __amigapath_into(in, buf);
  int ok = got == in;

  printf("%s %-60s -> %s\n", ok ? "ok  " : "FAIL", in,
         ok ? "(untouched)" : "(copied)");
  if (!ok)
    fails++;
}

int main(void)
{
  puts("default (NOIXPATHS unset): only . and .. are rewritten");
  check("Work:m68k-amigaos-gcc-16.2/bin/../lib/gcc/m68k-amigaos/16.2.0b/../../../../m68k-amigaos/lib/../libnix/include",
        "Work:m68k-amigaos-gcc-16.2/bin//lib/gcc/m68k-amigaos/16.2.0b/////m68k-amigaos/lib//libnix/include");
  check("a/../b", "a//b");
  check("a/./b", "a/b");
  check("./foo", "foo");
  check("a/..", "a//");
  check("..", "/");
  check("../foo", "/foo");
  check("../../foo", "//foo");
  check("Work:./foo", "Work:foo");
  check("Work:../foo", "Work:/foo");
  check("a/..foo/.hidden", "a/..foo/.hidden");
  /* native spellings pass through, and without copying */
  check_untouched("Work:a//b");
  check_untouched("/parent/file");
  check_untouched("T:cc1234.s");
  check_untouched("");

  __pathconv = 1;
  puts("NOIXPATHS=1: slashes collapse and /vol/path becomes vol:path");
  /* Unix "//" is one separator, then ".." is the AmigaDOS parent step */
  check("/Work/foo//bar/../baz", "Work:foo/bar//baz");
  check("/Work/foo", "Work:foo");
  check("/", "SYS:");
  check("//", "SYS:");
  check("a//b", "a/b");
  /* a relative parent path must not turn into a volume */
  check("../foo", "/foo");
  check("../../foo", "//foo");

  printf("%d failures\n", fails);
  return fails != 0;
}
