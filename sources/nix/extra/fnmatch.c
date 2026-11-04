/**
 * Coded by Stefan "Bebbo" Franke in 2019-2026. Replacement for all that copyrighted stuff.
 */
#include <ctype.h>
#include <fnmatch.h>
#include <string.h>
#include "stdio.h"

typedef int (*fxtype)(int);

#define FX(a) {#a, is##a}
static struct __fx {
	const char * name;
	fxtype fx;
} const data[] = { FX(alnum), FX(alpha), FX(blank), FX(cntrl), FX(digit), FX(lower), FX(print), FX(punct), FX(space), FX(upper), FX(xdigit), };

/**
 * Search the matching ctype function.
 */
static fxtype lookup(const char * name, size_t len) {
	if (len == 5) {
		size_t i;
		for (i = 0; i < sizeof(data) / sizeof(struct __fx) - 1; ++i)
			if (0 == strncmp(data[i].name, name, 5))
				return data[i].fx;
	} else if (len == 6) {
		int i = sizeof(data) / sizeof(struct __fx) - 1;
		if (0 == strncmp(data[i].name, name, 6))
			return data[i].fx;
	}
	return 0;
}

#define FLAG_PATHNAME (flags & FNM_PATHNAME)
#define	FLAG_NOESCAPE (flags & FNM_NOESCAPE)
#define FLAG_INPERIOD (flags & FNM_PERIOD)
#define FLAG_CASEFOLD (flags & FNM_CASEFOLD)

// Internal helper using a persistent global_start pointer across recursions
static int fnmatch_internal(const char *pattern, const char *string, const char *global_start, int flags) {
	unsigned char c = *string;
	unsigned char p = *pattern;

	while (p) {
		// POSIX: FNM_PERIOD restricts wildcards/brackets from matching '.'
		// ONLY if it is a true leading period relative to the base path component.
		int is_leading_period = 0;
		if (FLAG_INPERIOD && c == '.') {
			if (string == global_start || (FLAG_PATHNAME && string[-1] == '/')) {
				is_leading_period = 1;
			}
		}

		if (FLAG_CASEFOLD)
			c = tolower(c);

		switch (p) {
			case '*':
				// Collapse consecutive asterisks into a single one
				while (p == '*') {
					p = *++pattern;
				}

				// If pattern ends with '*', it matches the rest of the string
				if (!p) {
					if (FLAG_PATHNAME && strchr(string, '/'))
						return FNM_NOMATCH;
					if (FLAG_INPERIOD && *string == '.') {
						if (string == global_start || (FLAG_PATHNAME && string[-1] == '/'))
							return FNM_NOMATCH;
					}
					return 0;
				}

				// An asterisk matches 0 to N characters.
				// Recursively test every remaining suffix while maintaining global_start.
				while (*string) {
					if (!fnmatch_internal(pattern, string, global_start, flags))
						return 0;

					// Stop advancing the asterisk if hitting path or period boundaries
					if (FLAG_PATHNAME && *string == '/')
						break;
					if (FLAG_INPERIOD && *string == '.' &&
					   (string == global_start || (FLAG_PATHNAME && string[-1] == '/')))
						break;

					string++;
				}
				return FNM_NOMATCH;

			case '?':
				if (!c)
					return FNM_NOMATCH;
				if ((FLAG_PATHNAME && c == '/') || is_leading_period)
					return FNM_NOMATCH;
				break;

			case '[': {
				const char *start = pattern;
				short not, match;

				if (!c || (FLAG_PATHNAME && c == '/'))
					return FNM_NOMATCH;

				// A bracket expression must never match a genuine LEADING period
				if (is_leading_period)
					return FNM_NOMATCH;

				p = *++pattern;
				not = 0;
				if (p == '!' || p == '^') {
					not = 1;
					p = *++pattern;
				}

				// Look ahead to verify if the bracket expression is properly closed
				const char *end_check = pattern;
				if (*end_check == ']')
					end_check++; // A leading ']' inside the bracket counts as a literal character
				while (*end_check && *end_check != ']') {
					// the ']' of a [:class:] doesn't close the bracket
					if (end_check[0] == '[' && end_check[1] == ':') {
						const char *e = end_check + 2;
						while (*e && !(e[0] == ':' && e[1] == ']'))
							e++;
						if (*e) {
							end_check = e + 2;
							continue;
						}
					}
					// nor does an escaped one
					if (end_check[0] == '\\' && !FLAG_NOESCAPE && end_check[1])
						end_check++;
					end_check++;
				}

				// If no closing ']' is found, treat '[' as a literal character
				if (!*end_check) {
					pattern = start;
					p = *pattern;
					if (FLAG_CASEFOLD) {
						if (tolower(p) != c) return FNM_NOMATCH;
					} else if (p != c) {
						return FNM_NOMATCH;
					}
					break;
				}

				match = 0;
				do {
					// Handle POSIX character classes (e.g., [:digit:])
					if (p == '[' && pattern[1] == ':') {
						fxtype is;
						const char *name = pattern += 2;
						while (*pattern && *pattern != ':')
							++pattern;
						is = lookup(name, pattern - name);
						if (*pattern == 0 || *++pattern != ']') {
							match = not;
							break;
						}
						if (FLAG_CASEFOLD && is == isupper)
							is = islower;
						if (is)
							match |= is(c);
					}
					// A single character, or a range of them (e.g., a-z). An
					// escaped character is itself, at either end of a range too,
					// as the lookahead above took it.
					else {
						unsigned char from = p, to;
						if (from == '\\' && !FLAG_NOESCAPE && pattern[1])
							from = *++pattern;
						to = from;
						if (pattern[1] == '-' && pattern[2] && pattern[2] != ']') {
							pattern += 2;
							to = *pattern;
							if (to == '\\' && !FLAG_NOESCAPE && pattern[1])
								to = *++pattern;
						}
						if (FLAG_CASEFOLD) {
							from = tolower(from);
							to = tolower(to);
						}
						if (c >= from && c <= to)
							match = 1;
					}
					p = *++pattern;
				} while (p && p != ']');

				// never past the end of the pattern
				if (!p)
					return FNM_NOMATCH;
				if (match == not)
					return FNM_NOMATCH;
				break;
			}

			case '\\':
				if (!FLAG_NOESCAPE) {
					// Verify that another character actually follows the backslash
					if (pattern[1] != '\0') {
						p = *++pattern;
					}
					// A single trailing backslash at the very end of the pattern behaves as a literal '\'
				}
				/* fall through */

			default:
				if (FLAG_CASEFOLD) {
					if (tolower(p) != c)
						return FNM_NOMATCH;
				} else if (p != c) {
					return FNM_NOMATCH;
				}
				break;
		}

		p = *++pattern;
		if (c) {
			string++;
			c = *string;
		}
	}

	// Match is successful (0) only if both the pattern and the string reached their ends
	return (*string == '\0') ? 0 : FNM_NOMATCH;
}

// Public API wrapper
int fnmatch(const char *pattern, const char *string, int flags) {
	return fnmatch_internal(pattern, string, string, flags);
}
