/* The buffer size of stdin and stdout, which __initstdio() reads through
   this pointer: __iobufsize, unless a program points it elsewhere. */
extern unsigned long __iobufsize;
unsigned long *__BUFSIZE = &__iobufsize;
