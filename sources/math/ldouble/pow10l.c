extern double pow10(double x);

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double pow10l(long double x)
{
  return pow10((double)x);
}
