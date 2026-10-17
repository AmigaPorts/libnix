/*
 * The size of the buffer each stdio stream gets: fopen()ed files, stdin
 * and stdout.  Like __stack, a program can choose its own by defining it:
 *
 *   unsigned long __iobufsize = 65536;
 *
 * which keeps this default out of the link.  setvbuf() still changes the
 * buffer of a single stream.
 */
unsigned long __iobufsize = 8192;
