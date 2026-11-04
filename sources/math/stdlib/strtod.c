#include "stdio.h"

long double strtold(const char *nptr, char **endptr) {
	long double a = 0.0L;
	unsigned int n = 0;
	sscanf(nptr, "%Lf%ln", &a, &n);
	if (endptr)
		*endptr = (char*) nptr + n;
	return a;
}

double strtod(const char *nptr, char **endptr) {
	double a = 0.0L;
	unsigned int n = 0;
	sscanf(nptr, "%f%ln", &a, &n);
	if (endptr)
		*endptr = (char*) nptr + n;
	return a;
}

float strtof(const char *nptr, char **endptr) {
	float a = 0.0f;
	unsigned int n = 0;
	sscanf(nptr, "%f%ln", &a, &n);
	if (endptr)
		*endptr = (char*) nptr + n;
	return a;
}

