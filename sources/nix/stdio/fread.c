#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "stdio.h"

extern int __sprepread(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t subsize;
	size_t total;
	unsigned char *b = (unsigned char *) ptr;
	if (!(total = size * nmemb)) /* Just in case size==0 */
		return total;

	__STDIO_LOCK(stream);

	while (total) {
		if (stream->_r > 0) {
			subsize = total > (size_t) stream->_r ? (size_t) stream->_r : total;
			memcpy(b, stream->_p, subsize);
			stream->_p += subsize;
			stream->_r -= subsize;
			b += subsize;
			total -= subsize;
		} else if (!(stream->_flags & __SSTR)
			   && (stream->_flags & __SNBF
			       || total >= (size_t) stream->_bf._size)) {
			/* The buffer is empty and the request is at least a buffer,
			   or the stream is unbuffered: read straight into the
			   caller's memory.  A buffered stream reads whole buffers at
			   a time so it stays aligned to its buffer size; an
			   unbuffered one reads just what was asked, instead of a
			   byte or a buffer at a time.  One Read() instead of one per
			   buffer, each followed by a copy; on AmigaDOS every Read()
			   is a round trip to the handler.  */
			int n;
			if (__sprepread(stream))
				break;
			if (stream->_r > 0) /* ungetc() data came back */
				continue;
			stream->_flags |= __SRD;
			n = read(stream->file, b, stream->_flags & __SNBF
				 ? total : total - total % stream->_bf._size);
			if (n <= 0) {
				stream->_flags |= n ? __SERR : __SEOF;
				break;
			}
			b += n;
			total -= n;
		} else {
			int c;
			if ((c = __srget(stream)) == EOF)
				break;
			*b++ = c;
			total--;
		}
	}
	__STDIO_UNLOCK(stream);
	return (b - (unsigned char *) ptr) / size;
}
