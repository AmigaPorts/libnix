#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <proto/dos.h>

/**
 * Reopen a file.
 *
 * @return file stream.
 */
extern int __fclose(FILE *);

extern StdFileDes **__stdfiledes;

/* __stdLock before the stream's lock, as everywhere: open() takes it. */
FILE *freopen(const char *filename, const char *mode, FILE *stream) {
	__GLOBAL_STDIO_LOCK;
	__STDIO_LOCK(stream);

	int error = __fclose(stream);
	if (error)
		goto fail;

	if (filename != NULL) {
		char ch;
		long file, flags;
		if (mode == NULL)
			goto fail;

		ch = *mode++;
		switch (ch) {
			case 'r':
				flags = O_RDONLY;
				break;
			case 'w':
				flags = O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case 'a':
				flags = O_WRONLY | O_CREAT | O_APPEND;
				break;
			default:
				goto fail;
		}

		ch = *mode++;
		if (ch == 't' || ch == 'b') // ignore for compatibility
			ch = *mode++;

		if (ch == '+')
			flags = (flags & ~O_ACCMODE) | O_RDWR;

		if ((file = open(filename, flags, 0777)) < 0)
			goto fail;

		if (flags & O_APPEND)
			Seek(__stdfiledes[stream->file]->lx_fh, 0, OFFSET_END);

		/* clear a lot of flags */
		stream->_flags &= ~(__SWO | __SERR | __SEOF | __SWR | __SRD | __SNBF | __SLBF);
		if (flags & O_WRONLY)
			stream->_flags |= __SWO; /* set write-only flag */
		if (isatty(file))
			stream->_flags |= __SLBF; /* set linebuffered flag */
		stream->file = file;
	}

	__STDIO_UNLOCK(stream);
	__GLOBAL_STDIO_UNLOCK;
	return stream;

fail:
	__STDIO_UNLOCK(stream);
	__GLOBAL_STDIO_UNLOCK;
	return NULL;
}
