#include <stdio.h>

float strtof(const char *nptr, char **endptr) {
	float a = 0.0f;
	long n = 0;
	sscanf(nptr, "%f%ln", &a, &n);

	if (endptr)
		*endptr = (char*) nptr + n;

	return a;
}

