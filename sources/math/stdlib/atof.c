#include "stdio.h"

double atof(const char *nptr)
{ double a=0.0;
  sscanf(nptr,"%lf",&a);
  return a;
}
