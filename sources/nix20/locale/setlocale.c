#include <limits.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <dos/dos.h>
#ifdef __posix_threads__
#include <exec/semaphores.h>
#endif
#ifndef __KICK13__
#include <libraries/locale.h>
#include <proto/locale.h>
#endif
#include <proto/exec.h>
#include "stabs.h"

extern struct LocaleBase *LocaleBase;
//extern char __localename[];
extern struct Locale *__localevec[];

/* for LC_CTYPE */
extern const unsigned char _ctype___data[];
__near
extern const unsigned char * _ctype_;
static unsigned char *ctype;

/* for LC_NUMERIC */
extern char *__decimalpoint;

/* for LC_TIME */
extern long __timezone__data;
//extern int _daylight;

struct lconv __lconv;

int __locale_mb_cur_max(void) {
	return 0xff;
}

/* Keep returned names and opened locales alive while callers use them.
   Thread-enabled builds serialize individual setlocale() calls.  */
#ifdef __posix_threads__
static struct SignalSemaphore __localesema;
static volatile int __localesema_ready;
#endif

struct __localename {
	struct __localename *next;
	char name[1];
};
static struct __localename *__localenames; /* every name returned */

#ifndef __KICK13__
struct __localeopen {
	struct __localeopen *next;
	struct Locale *locale;
	char name[1];
};
static struct __localeopen *__localesopen; /* every locale opened */
static struct Locale *__ctypelocale; /* the locale ctype[] was made from */
#endif

static void __localelock(void) {
#ifdef __posix_threads__
	if (!__localesema_ready) {
		Forbid();
		if (!__localesema_ready) {
			InitSemaphore(&__localesema);
			__localesema_ready = 1;
		}
		Permit();
	}
	ObtainSemaphore(&__localesema);
#endif
}

static void __localeunlock(void) {
#ifdef __posix_threads__
	ReleaseSemaphore(&__localesema);
#endif
}

/* the kept copy of name: the same string every time for the same name */
static char *__keepname(const char *name) {
	struct __localename *n;
	for (n = __localenames; n != NULL; n = n->next)
		if (strcmp(n->name, name) == 0)
			return n->name;
	if ((n = malloc(sizeof(*n) + strlen(name))) == NULL)
		return NULL;
	strcpy(n->name, name);
	n->next = __localenames;
	__localenames = n;
	return n->name;
}

#ifndef __KICK13__
/* the locale called name ("" for the user's default), opened once */
static struct Locale *__openlocale(const char *name) {
	struct __localeopen *o;
	struct Locale *locale;
	if (name[0] != '\0')
		for (o = __localesopen; o != NULL; o = o->next)
			if (strcmp(o->name, name) == 0 || (o->locale->loc_LocaleName != NULL
					&& strcmp((const char *) o->locale->loc_LocaleName, name) == 0))
				return o->locale;
	if ((locale = OpenLocale(name[0] == '\0' ? NULL : (STRPTR) name)) == NULL)
		return NULL;
	for (o = __localesopen; o != NULL; o = o->next)
		if (o->locale == locale) { /* the default locale again */
			CloseLocale(locale);
			return locale;
		}
	if ((o = malloc(sizeof(*o) + strlen(name))) == NULL) {
		CloseLocale(locale);
		return NULL;
	}
	strcpy(o->name, name);
	o->locale = locale;
	o->next = __localesopen;
	__localesopen = o;
	return locale;
}
#endif

static char *__getlocalename(size_t a, size_t b) {
	size_t i, s = 0;
	char *string, *name;

	for (i = a; i <= b; i++) {
#ifndef __KICK13__
		if (__localevec[i] != NULL)
			s += strlen((const char *) __localevec[i]->loc_LocaleName) + 1;
		else
#endif
			s += 2; /* "C" */
	}

	if ((string = malloc(s)) == NULL)
		return NULL;

	string[0] = '\0';
	for (i = a; i <= b; i++) {
#ifndef __KICK13__
		if (__localevec[i] != NULL)
			strcat(string, (const char *) __localevec[i]->loc_LocaleName);
		else
#endif
			strcat(string, "C");
		if (i != b)
			strcat(string, "\n");
	}
	name = __keepname(string);
	free(string);
	return name;
}

static char *__setlocale(size_t a, size_t b, const char *name) {
	size_t i;
	struct Locale *vec[5];
	char *string, *s1 __attribute__((unused)), *s2, c; /* s1 is unused with __KICK13__ */

	if ((string = strdup(name)) == NULL)
		return NULL;

	/* one name for all the categories, or one per category, each ended by
	   a newline, as __getlocalename() makes them */
	s1 = s2 = string;
	for (i = a; i <= b; i++) {
		while (*s2 != '\0' && *s2 != '\n')
			s2++;
		c = *s2;
		*s2 = '\0';
#ifndef __KICK13__
		if (LocaleBase == NULL || (s1[0] == 'C' && s1[1] == '\0')) /* This is the only place */
			vec[i] = NULL; /* LocaleBase gets tested for NULL */
		else if ((vec[i] = __openlocale(s1)) == NULL) {
			free(string);
			return NULL; /* nothing has changed */
		}
#else
		vec[i] = NULL;
#endif
		*s2 = c;
		if (c == '\0')
			s2 = string;
		else
			s2++;
		s1 = s2;
	}
	free(string);

	for (i = a; i <= b; i++)
		__localevec[i] = vec[i]; /* closed at exit, not now: another thread may use it */

	// set defaults
	__lconv.decimal_point = ".";
	__lconv.thousands_sep = "";

#ifndef __KICK13__
	if (__localevec[LC_CTYPE - 1] != NULL && ctype != NULL) {
		struct Locale *locale = __localevec[LC_CTYPE - 1];
		if (locale != __ctypelocale) {
			ctype[0] = 0;
			for (i = 0; i < 256; i++)
				ctype[i + 1] = ((IsPrint(locale,i) && !IsGraph(locale, i)) ? 128 : 0) | (IsXDigit(locale,i) ? 64 : 0) | (IsCntrl(locale,i) ? 32 : 0) | (IsPunct(locale,i) ? 16 : 0)
						| (IsSpace(locale,i) ? 8 : 0) | (IsDigit(locale,i) ? 4 : 0) | (IsLower(locale,i) ? 2 : 0) | (IsUpper(locale,i) ? 1 : 0);
			__ctypelocale = locale;
		}
		_ctype_ = ctype;
	} else
#endif
		_ctype_ = _ctype___data;

#ifndef __KICK13__
	if (__localevec[LC_MONETARY - 1] != NULL) {
		struct Locale *locale = __localevec[LC_MONETARY - 1];
		__lconv.int_curr_symbol = (char *) locale->loc_MonIntCS;
		__lconv.currency_symbol = (char *) locale->loc_MonCS;
		__lconv.mon_decimal_point = (char *) locale->loc_MonDecimalPoint;
		__lconv.mon_thousands_sep = (char *) locale->loc_MonGroupSeparator;
		__lconv.mon_grouping = (char *) locale->loc_MonFracGrouping;
		__lconv.positive_sign = (char *) locale->loc_MonPositiveSign;
		__lconv.negative_sign = (char *) locale->loc_MonNegativeSign;
		__lconv.int_frac_digits = locale->loc_MonIntFracDigits;
		__lconv.frac_digits = locale->loc_MonFracDigits;
		__lconv.p_cs_precedes = locale->loc_MonPositiveCSPos;
		__lconv.p_sep_by_space = locale->loc_MonPositiveSpaceSep;
		__lconv.p_sign_posn = locale->loc_MonPositiveSignPos;
		__lconv.n_cs_precedes = locale->loc_MonNegativeCSPos;
		__lconv.n_sep_by_space = locale->loc_MonNegativeSpaceSep;
		__lconv.n_sign_posn = locale->loc_MonNegativeSignPos;
	} else
#endif
	{
		__lconv.int_curr_symbol = "";
		__lconv.currency_symbol = "";
		__lconv.mon_decimal_point = "";
		__lconv.mon_thousands_sep = "";
		__lconv.mon_grouping = "";
		__lconv.positive_sign = "";
		__lconv.negative_sign = "";
		__lconv.int_frac_digits = CHAR_MAX;
		__lconv.frac_digits = CHAR_MAX;
		__lconv.p_cs_precedes = CHAR_MAX;
		__lconv.p_sep_by_space = CHAR_MAX;
		__lconv.p_sign_posn = CHAR_MAX;
		__lconv.n_cs_precedes = CHAR_MAX;
		__lconv.n_sep_by_space = CHAR_MAX;
		__lconv.n_sign_posn = CHAR_MAX;
	}

	if (__localevec[LC_NUMERIC - 1] != NULL) {
		struct Locale *locale __attribute__((unused)) = __localevec[LC_NUMERIC - 1]; /* unused with __KICK13__ */
#ifndef __KICK13__
		__lconv.decimal_point = (char*) locale->loc_DecimalPoint;
#else
		__lconv.decimal_point = (char*) ".";
#endif
#ifndef __KICK13__
		__lconv.thousands_sep = (char*) locale->loc_GroupSeparator;
#else
		__lconv.thousands_sep = (char*)",";
#endif
	}
	/* "C" gives printf() and scanf() the point back too */
	__decimalpoint = __lconv.decimal_point;

#ifndef __KICK13__
	if (__localevec[LC_TIME - 1] != NULL)
		__timezone__data = __localevec[LC_TIME - 1]->loc_GMTOffset * 60;
	else
#endif
		__timezone__data = 0;

	return (char *) name;
}

char *setlocale(int category, const char *name) {
	size_t a, b;
	char *result;

	if (category < 0 || category > 5)
		return NULL;

	if (category == LC_ALL) {
		a = 0;
		b = 4;
	} else
		a = b = category - 1;

	__localelock();
	if (name == NULL) /* return name of current locale */
		result = __getlocalename(a, b);
	else
		result = __setlocale(a, b, name);
	__localeunlock();
	return result;
}

void __initlocale(void) {
	if ((ctype = malloc(257)) != NULL) {
		if (setlocale(LC_ALL, "") != NULL) /* Set the default locale */
			return;
	}
}

void __exitlocale(void) {
	struct __localename *n;
#ifndef __KICK13__
	struct __localeopen *o;
	while ((o = __localesopen) != NULL) {
		__localesopen = o->next;
		CloseLocale(o->locale);
		free(o);
	}
#endif
	while ((n = __localenames) != NULL) {
		__localenames = n->next;
		free(n);
	}
}

ADD2INIT(__initlocale, -10);
ADD2EXIT(__exitlocale, -10);
