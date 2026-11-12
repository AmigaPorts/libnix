#include <exec/types.h>
#include <clib/alib_protos.h>

/* ACrypt() of amiga.lib: hashes a password and a user name into 11 printable
   characters.  buffer holds 12 bytes.  The results are those of the original
   amiga.lib, which password files store. */

#define LEN 12

STRPTR ACrypt(STRPTR buffer, CONST_STRPTR password, CONST_STRPTR user)
{
  UWORD mix[LEN];
  UWORD i,k;

  /* a string that ends early goes on as the position */
  for (i=0; i<LEN; i++) {
    UWORD c = 'A';
    c += *password ? *password++ : i;
    c += *user ? *user++ : i;
    mix[i] = c;
  }

  /* every sum fits 16 bits: no call to the long division of libgcc */
  for (i=0; i<LEN; i++) {
    for (k=LEN; k-->0;)
      mix[i] = (UWORD)(mix[i]+mix[k]) % (UWORD)53;
    buffer[i] = 'A'+mix[i];
  }
  buffer[LEN-1] = 0;

  return buffer;
}
