#include "stdio.h"

long double strtold(const char *nptr, char **endptr) {
	long double a = 0.0L;
	unsigned long n = 0;
	sscanf(nptr, "%Lf%ln", &a, &n);
	if (endptr)
		*endptr = (char*) nptr + n;
	return a;
}

double strtod(const char *nptr, char **endptr) {
	return strtold(nptr, endptr);
}

float strtof(const char *nptr, char **endptr) {
	return (float) strtold(nptr, endptr);
}

