#include <errno.h>
#include "stdio.h"

extern int __fflush(FILE *);

/*
 * __swbuf - Internal buffered output function
 *
 * This function is called by putc() and similar functions when the buffer
 * is full or when special handling is needed (line buffering, unbuffered).
 *
 * The key insight is that stream->_w stores a combined value:
 *   - For full buffering:   _w = actual space left in buffer
 *   - For line buffering:   _w = actual space left + linebufsize
 *                           where linebufsize = -_bf._size
 *   - For unbuffered:       _w = -1 (no buffer space)
 *
 * This allows putc() to check if there's space with a single decrement
 * operation in the common case.
 *
 * Parameters:
 *   c      - Character to output
 *   stream - Output stream
 *
 * Returns:
 *   The character written on success, EOF on error
 */
int __swbuf(int c, FILE *stream)
{
    int combined_w;      /* Combined write count (space + linebufsize) */
    int linebuf_adjust;  /* Adjustment for line buffering (-bufsize or 0) */
    short flags;         /* Saved stream flags */
    int result = c;      /* Return value, default to c */

    /* Save current flags for later restoration */
    flags = stream->_flags;

    /* Lock the stream for thread safety */
    __STDIO_LOCK(stream);

    /*
     * Check for error conditions that prevent writing:
     * 1. String stream (sprintf buffer) - not writable
     * 2. Error already set on stream
     * 3. Stream is in read mode
     */
    if (flags & (__SSTR | __SERR | __SRD)) {
        /* String buffer or error - can't write */
        if (flags & (__SSTR | __SERR)) {
            stream->_w = 0;
            errno = EPERM;
            __STDIO_UNLOCK(stream);
            return EOF;
        }

        /*
         * Was in read mode - switch to write mode
         * Discard any input buffer data
         */
        if (flags & __SRD) {
            stream->_r = 0;          /* Throw away input buffer */
            stream->tmpp = NULL;      /* Clear ungetc state */
            stream->_flags = flags &= ~__SRD;  /* Clear read mode flag */
        }
    }

    /*
     * Calculate buffer space:
     *
     * For line buffering: linebuf_adjust = -buffer_size
     *   This makes combined_w negative when buffer is full
     *   Example: buffer_size=1024, full buffer: combined_w = -1
     *
     * For full buffering: linebuf_adjust = 0
     *   combined_w = actual space left
     *   Example: buffer_size=1024, full buffer: combined_w = 0
     *
     * For unbuffered: buffer_space = 0, combined_w = -1
     */
    linebuf_adjust = (flags & __SLBF) ? -stream->_bf._size : 0;
    combined_w = ((flags & __SNBF) ? 0 : stream->_bf._size - 1) + linebuf_adjust;

    /*
     * Initialize buffer if stream wasn't in write mode
     *
     * Note: --combined_w pre-decrements to reserve space for the
     * character we're about to write
     */
    if (!(flags & __SWR)) {
        stream->_p = stream->_bf._base;    /* Point to start of buffer */
        stream->_w = --combined_w;         /* Set initial combined write count */
        stream->_flags = flags |= __SWR;   /* Mark as write mode */
    }

    /* Write the character to the buffer */
    *stream->_p++ = c;

    /*
     * Determine if we need to flush:
     *
     * The flush condition depends on buffering mode:
     *
     * For full buffering:
     *   Flush when _w < 0 (buffer full)
     *   Condition: stream->_w < 0 && (stream->_w < lbs || true)
     *
     * For line buffering:
     *   Flush when buffer full OR newline character
     *   stream->_w < 0 is always true (due to linebuf_adjust)
     *   Condition: stream->_w < 0 && (stream->_w < lbs || c == '\n')
     *   - stream->_w < lbs means actual space left < 0 (buffer full)
     *   - c == '\n' means newline character
     *
     * For unbuffered:
     *   Flush on every character (combined_w = -1, so _w < 0 always)
     */
    if (stream->_w < 0 && (stream->_w < linebuf_adjust || (char)c == '\n')) {
        /*
         * Flush the buffer to the underlying file descriptor
         *
         * Important: __fflush() manages its own locking and will:
         * 1. Write the buffer contents
         * 2. Clear __SWR flag
         * 3. Reset _w to 0
         * 4. Reset linebufsize to 0
         */
        if (__fflush(stream) != 0) {
            /* __fflush already set __SERR flag */
            __STDIO_UNLOCK(stream);
            return EOF;
        }

        /*
         * Reset buffer state after flush
         *
         * Note: We don't blindly restore all flags because __fflush may
         * have set error flags. Only restore if no error occurred.
         */
        stream->_p = stream->_bf._base;  /* Reset to start of buffer */
        if (!(stream->_flags & __SERR)) {
            stream->_flags = flags;       /* Restore original flags (including __SWR) */
        }

        /*
         * Recalculate combined_w for the next character
         * Note: combined_w was pre-decremented above, so we need to
         * recalculate the original value for the new buffer
         */
        linebuf_adjust = (flags & __SLBF) ? -stream->_bf._size : 0;
        combined_w = ((flags & __SNBF) ? 0 : stream->_bf._size - 1) + linebuf_adjust;
        stream->_w = combined_w;          /* Reset to full buffer state */
    } else {
        /*
         * No flush needed - update state for next character
         *
         * For line buffering, we need to maintain the combined value:
         *   _w = actual_space_left + linebuf_adjust
         *
         * For full buffering: _w = actual_space_left
         * For unbuffered: _w = -1 (always needs flush)
         */
        stream->linebufsize = linebuf_adjust;
        stream->_w = combined_w;  /* combined_w is already pre-decremented */
    }

    __STDIO_UNLOCK(stream);

    return result;
}
