#include <stdlib.h>
#include <errno.h>
#include <wchar.h>
#include "stdio.h"

int __fflush(FILE *stream);
extern rspinlock_t __stdLock;
wint_t ungetwc(wint_t c, FILE *stream) {
	size_t i;
	wint_t cc;
	wint_t r = (wint_t) EOF;

	__recursiveSpinLock(&__stdLock);
	do { // while (0);
		if (c == (wint_t) EOF)
			break;
		if (stream->_flags & __SERR)/* Error on stream */
		{
			errno = EPERM;
			break;
		}
		if (stream->_flags & __SWR) /* File is in write mode */
			if (__fflush(stream)) /* write buffer */
				break;
		if (stream->tmpp == NULL) {
			if (stream->_flags & __SRD) /* File is in read mode */
			{
				stream->tmpp = stream->_p; /* store actual position */
				stream->tmpinc = stream->_r;
				stream->_p = &stream->unget[4]; /* and use other buffer */
				stream->_r = 0;
			} else /* File is in indeterminated state */
			{
				stream->tmpp = (unsigned char*) 2; /* Dummy */
				stream->tmpinc = 0;
				stream->_p = &stream->unget[4]; /* use other buffer */
				stream->_r = 0;
				stream->_flags |= __SRD;
			}
		}
		if ((wint_t) stream->_r >= 4 + 1 - sizeof(wint_t)) /* ungetc buffer overflow */
			break;
		stream->_r += sizeof(wint_t);

		cc = c;
		for (i = 0; i < sizeof(wint_t); ++i) {
			*--stream->_p = cc;
			cc >>= 8;
		}
		r = c;
	}
	while (0);
	__recursiveSpinUnlock(&__stdLock);
	return r;
}
