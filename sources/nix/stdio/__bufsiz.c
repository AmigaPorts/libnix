/* The size of the buffer every stream libnix opens gets: fopen()ed files,
   stdin and stdout.  8 KB rather than the public BUFSIZ (1 KB), which
   programs size their own buffers with, often on a 4 KB stack: fewer
   Read()/Write() calls per byte, and the Amiga filesystem handlers make
   each one expensive.  8 KB is where sequential I/O stops gaining on RAM:
   (PR #47).

   Like __stack, a program can choose its own by defining both

     unsigned __BUFSIZ__data = 65536;
     unsigned *__BUFSIZE = &__BUFSIZ__data;

   which keeps this object out of the link.  setvbuf() still changes the
   buffer of a single stream. */
unsigned __BUFSIZ__data = 8192;
unsigned *__BUFSIZE = &__BUFSIZ__data;
