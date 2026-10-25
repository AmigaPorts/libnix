#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "stdio.h"
#include "stabs.h"

signed long wcstol(const wchar_t *nptr, wchar_t **endptr, int base) {
	const wchar_t *p = nptr;
	wchar_t *q;
	char *_q;
	unsigned long r;
	if (!(nptr && *nptr)) {
		if (endptr != NULL)
			*endptr = (wchar_t*) nptr;
		errno = EINVAL;
		return 0;
	}
	while ((unsigned long) *p < 128 && isspace((int )*p))
		p++;

	int l = wcslen(p);
	char *_p = (char*) malloc(l + 1);
	if (!_p) {
		if (endptr != NULL)
			*endptr = (wchar_t*) nptr;
		errno = ENOMEM;
		return 0;
	}

	// convert wchar_t to char, ending at the first non-ASCII character,
	// which cannot be part of a number
	int i;
	for (i = 0; i < l && (unsigned long) p[i] < 128; ++i)
		_p[i] = p[i];
	_p[i] = '\0';

	/* strtoul's overflow is ULONG_MAX, unnegated: the same value as "-1" */
	int olderrno = errno, overflow;
	errno = 0;
	r = strtoul(_p, &_q, base);
	overflow = errno == ERANGE;
	if (!errno)
		errno = olderrno;
	free(_p);

	// translate end
	q = (wchar_t*)p + (_q - _p);

	if (endptr != NULL) {
		if (q == p)
			*endptr = (wchar_t*) nptr;
		else
			*endptr = q;
	}
	if (*p == '-') {
		if (overflow || (signed long) r > 0) {
			errno = ERANGE;
			return LONG_MIN;
		} else
			return r;
	} else {
		if ((signed long) r < 0) {
			errno = ERANGE;
			return LONG_MAX;
		} else
			return r;
	}
}
