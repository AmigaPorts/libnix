extern double pow10(double x, double y);

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double pow10l(long double x, long double y)
{
  return pow10((double)x, (double)y);
}
