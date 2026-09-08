#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dos/var.h>
#include <proto/dos.h>

#ifndef GVF_LOCAL_ONLY
#define GVF_LOCAL_ONLY 0x200
#endif

extern char ** environ_ptr__data;
extern char *__dummy_env[];

/* The variables are mirrored into the process's local variables: children
 * run through System() inherit those, and __fillenviron reads them back
 * into environ at startup, so the environment reaches child processes the
 * way it does on Unix. environ keeps its POSIX case-sensitive names; the
 * local variables are case-insensitive, so names differing only in case
 * share one of them and the child sees the last one set. */

/* Remove the entry at p, shifting the rest of the array down. */
static void __removeenv(char **p) {
	free(*p);
	do {
		p[0] = p[1];
	} while (*p++);
}

int unsetenv(const char *name) {
	int l;
	char **p;

	if (!name || !*name || strchr(name, '=')) {
		errno = EINVAL;
		return -1;
	}

	l = strlen(name);
	p = environ_ptr__data;
	while (*p) {
		if (0 == strncmp(*p, name, l) && (*p)[l] == '=')
			__removeenv(p);
		else
			++p;
	}
	DeleteVar((CONST_STRPTR)name, GVF_LOCAL_ONLY | LV_VAR);
	return 0;
}

int setenv(const char *name, const char *value, int overwrite) {
	int l;
	char *entry, **p, **neu;

	if (!name || !*name || !value || strchr(name, '=')) {
		errno = EINVAL;
		return -1;
	}

	l = strlen(name);
	for (p = environ_ptr__data; *p; ++p) {
		if (0 == strncmp(*p, name, l) && (*p)[l] == '=')
			break;
	}
	if (*p && !overwrite)
		return 0;

	entry = concat(name, "=", value, 0);
	if (!entry) {
		errno = ENOMEM;
		return -1;
	}

	if (!*p) {
		/* insert: p points at the terminator, make room for the new entry
		 * and a new terminator */
		if (environ_ptr__data != __dummy_env)
			neu = (char **)realloc(environ_ptr__data, (p - environ_ptr__data + 2) * sizeof(char *));
		else
			neu = (char **)malloc(2 * sizeof(char *));
		if (!neu) {
			free(entry);
			errno = ENOMEM;
			return -1;
		}
		if (environ_ptr__data != __dummy_env)
			p = neu + (p - environ_ptr__data);
		else
			p = neu;
		environ_ptr__data = neu;
		p[1] = 0;
	}

	/* mirror first: on failure environ stays as it was (the slot made for
	 * an insert still holds the terminator) */
	if (!SetVar((CONST_STRPTR)name, (CONST_STRPTR)value, -1, GVF_LOCAL_ONLY | LV_VAR)) {
		free(entry);
		errno = ENOMEM;
		return -1;
	}

	if (*p)
		free(*p);
	*p = entry;
	return 0;
}

int putenv(const char *str) {
	int retval = -1;
	if (str && *str) {
		char *tmp = strdup(str);
		if (!tmp) {
			errno = ENOMEM;
		} else {
			char * pos;
			pos = strchr(tmp, '=');
			if (pos) {
				*pos++ = '\0';
				retval = setenv(tmp, pos, 1);
			} else
				errno = EINVAL;
			free(tmp);
		}
	} else
		errno = EINVAL;
	return retval;
}
