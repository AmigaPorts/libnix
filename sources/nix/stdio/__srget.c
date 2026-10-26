#include <errno.h>
#include <unistd.h>

#include "stdio.h"

#include "__filenode.h"

extern int __fflush(FILE*);

/* Get a stream ready to read from its file: fail on a stream in error or
   at EOF, flush pending output, and bring back ungetc() data: when some is
   left, return 0 at once, string stream or not, with the data in the
   buffer (_r > 0) for the caller to take.  Otherwise refuse a string
   stream, and flush the line buffered output streams before reading an
   unbuffered or line buffered one (ANSI).  Returns EOF when there is
   nothing to read; the caller holds the stream lock.  fread() uses it to
   read straight into a caller's memory; __srget() to refill the buffer.  */
int __sprepread(FILE *stream)
{
	if (stream->_flags & (__SERR | __SEOF)) /* Error on stream / EOF */
	{
		stream->_r = 0;
		errno = EPERM;
		return EOF;
	}
	if (stream->_flags & __SWR) {
		if (__fflush(stream))
			return EOF;
	} else if (stream->tmpp != NULL) /* File is in ungetc mode */
	{
		stream->_p = stream->tmpp;
		stream->_r = stream->tmpinc;
		stream->tmpp = NULL;
		/* left from before the ungetc(): hand it out first, or a string
		   stream (sscanf) would end at the __SSTR test below */
		if (stream->_r > 0)
			return 0;
	}
	if (stream->_flags & __SSTR) /* it's a sscanf buffer */
		return EOF;
	if (stream->_flags & (__SNBF | __SLBF)) /* Before reading from line- or unbuffered input file */
	{ /* fflush all line buffered output files (ANSI) */
		struct filenode *fp = (struct filenode*) ___filelist.mlh_Head;
		while (fp->node.mln_Succ) {
			if ((fp->theFILE._flags & (__SWR | __SLBF)) == (__SWR | __SLBF))
				__fflush(&fp->theFILE); /* Don't return EOF if this fails */
			fp = (struct filenode*) fp->node.mln_Succ;
		}
	}
	return 0;
}

int __srget(FILE *stream) /* Get next input block */
{
	int r;
	__STDIO_LOCK(stream);

	if (__sprepread(stream)) {
		__STDIO_UNLOCK(stream);
		return EOF;
	}
	if (stream->_r > 0) /* ungetc() data */
	{
		stream->_r--;
		r = *stream->_p++;
		__STDIO_UNLOCK(stream);
		return r;
	}
	stream->_flags |= __SRD;
	stream->_r = read(stream->file, stream->_bf._base, stream->_bf._size);
	if (!stream->_r) /* EOF found */
	{
		stream->_flags |= __SEOF;
		__STDIO_UNLOCK(stream);
		return EOF;
	} else if (stream->_r < 0) /* Error */
	{
		stream->_r = 0;
		stream->_flags |= __SERR;
		__STDIO_UNLOCK(stream);
		return EOF;
	}
	stream->_r--;
	stream->_p = stream->_bf._base;
	r = *stream->_p++;
	__STDIO_UNLOCK(stream);
	return r;
}
