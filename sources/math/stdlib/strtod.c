#include <stdio.h>

double strtod(const char *nptr, char **endptr) {
	double a = 0.0L;
	long n = 0;
	sscanf(nptr, "%lf%ln", &a, &n);

	if (endptr)
		*endptr = (char*) nptr + n;

	return a;
}
