#include <float.h>
#include <math.h>
#include "math_errno.h"

#define NAN_DOUBLE  __builtin_nan("")
#define NAN_FLOAT   __builtin_nanf("")
#define INF_DOUBLE  __builtin_huge_val()
#define INF_FLOAT   __builtin_huge_valf()

static double range(double result)
{
  errno = ERANGE;
  return result;
}

static float rangef(float result)
{
  errno = ERANGE;
  return result;
}

/* A NaN goes through, and anything else is a domain error.  */
double __math_domain(double x)
{
  if (__math_nan_double(x))
    return x;
  errno = EDOM;
  return NAN_DOUBLE;
}

float __math_domainf(float x)
{
  if (__math_nan_float(x))
    return x;
  errno = EDOM;
  return NAN_FLOAT;
}

double __math_overflow(double sign)
{
  return range(__math_negative(sign) ? -INF_DOUBLE : INF_DOUBLE);
}

float __math_overflowf(float sign)
{
  return rangef(__math_negative(sign) ? -INF_FLOAT : INF_FLOAT);
}

double __math_underflow(double sign)
{
  return range(__math_negative(sign) ? -0.0 : 0.0);
}

float __math_underflowf(float sign)
{
  return rangef(__math_negative(sign) ? -0.0f : 0.0f);
}

double __math_log_special(double x)
{
  if (__math_zero_double(x))
    return range(-INF_DOUBLE);
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  return __math_domain(x);
}

float __math_log_specialf(float x)
{
  if (__math_zero_float(x))
    return rangef(-INF_FLOAT);
  if (__math_inf_float(x) && !__math_negative(x))
    return x;
  return __math_domainf(x);
}

double __math_sqrt_special(double x)
{
  if (__math_zero_double(x) || (__math_inf_double(x) && !__math_negative(x)))
    return x;
  return __math_domain(x);
}

float __math_sqrt_specialf(float x)
{
  if (__math_zero_float(x) || (__math_inf_float(x) && !__math_negative(x)))
    return x;
  return __math_domainf(x);
}

double __math_exp_special(double x)
{
  if (__math_nan_double(x))
    return x;
  if (__math_inf_double(x))
    return __math_negative(x) ? 0.0 : x;
  return __math_negative(x) ? __math_underflow(1.0) : __math_overflow(1.0);
}

float __math_exp_specialf(float x)
{
  if (__math_nan_float(x))
    return x;
  if (__math_inf_float(x))
    return __math_negative(x) ? 0.0f : x;
  return __math_negative(x) ? __math_underflowf(1.0f) : __math_overflowf(1.0f);
}

double __math_expm1_special(double x)
{
  if (__math_nan_double(x))
    return x;
  if (__math_inf_double(x))
    return __math_negative(x) ? -1.0 : x;
  return __math_overflow(1.0);
}

float __math_expm1_specialf(float x)
{
  if (__math_nan_float(x))
    return x;
  if (__math_inf_float(x))
    return __math_negative(x) ? -1.0f : x;
  return __math_overflowf(1.0f);
}

double __math_log1p_special(double x)
{
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  if (__math_one_double(x) && __math_negative(x))
    return range(-INF_DOUBLE);
  return __math_domain(x);
}

float __math_log1p_specialf(float x)
{
  if (__math_inf_float(x) && !__math_negative(x))
    return x;
  if (__math_one_float(x) && __math_negative(x))
    return rangef(-INF_FLOAT);
  return __math_domainf(x);
}

double __math_acosh_special(double x)
{
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  return __math_domain(x);
}

float __math_acosh_specialf(float x)
{
  if (__math_inf_float(x) && !__math_negative(x))
    return x;
  return __math_domainf(x);
}

double __math_atanh_special(double x)
{
  if (__math_one_double(x))
    return __math_overflow(x);
  return __math_domain(x);
}

float __math_atanh_specialf(float x)
{
  if (__math_one_float(x))
    return __math_overflowf(x);
  return __math_domainf(x);
}

/* For fmod(), remainder() and remquo(), with x or y special or y zero.  */
double __math_fmod_special(double x, double y)
{
  if (__math_nan_double(x))
    return x;
  if (__math_nan_double(y))
    return y;
  if (__math_inf_double(x) || __math_zero_double(y))
    return __math_domain(x);
  return x;
}

float __math_fmod_specialf(float x, float y)
{
  if (__math_nan_float(x))
    return x;
  if (__math_nan_float(y))
    return y;
  if (__math_inf_float(x) || __math_zero_float(y))
    return __math_domainf(x);
  return x;
}

/* With x, y or z NaN or infinite.  */
double __math_fma_special(double x, double y, double z)
{
  int negative;

  if (__math_nan_double(x))
    return x;
  if (__math_nan_double(y))
    return y;
  if (__math_nan_double(z))
    return z;
  if (!__math_inf_double(x) && !__math_inf_double(y))
    return z;
  if (__math_zero_double(x) || __math_zero_double(y))
    return __math_domain(x);
  negative = __math_negative(x) != __math_negative(y);
  if (__math_inf_double(z) && negative != __math_negative(z))
    return __math_domain(x);
  return negative ? -INF_DOUBLE : INF_DOUBLE;
}

/* 0 for a number with a fraction, 1 for an even integer, 2 for an odd one.  */
int __math_integer(double x)
{
  int power = __math_power_double(x), shift;
  unsigned long high, low = __math_low(x);

  if (power < 0)
    return 0;
  if (power >= 53)
    return 1;
  high = (__math_high(x) & 0xfffffUL) | 0x100000UL;
  if (power <= 20) {
    shift = 20 - power;
    if (low != 0 || (high & ((1UL << shift) - 1)) != 0)
      return 0;
    return (high >> shift) & 1 ? 2 : 1;
  }
  if (power == 52)
    return low & 1 ? 2 : 1;
  shift = 52 - power;
  if ((low & ((1UL << shift) - 1)) != 0)
    return 0;
  return (low >> shift) & 1 ? 2 : 1;
}

int __math_integerf(float x)
{
  int power = __math_power_float(x), shift;
  unsigned long bits;

  if (power < 0)
    return 0;
  if (power >= 24)
    return 1;
  shift = 23 - power;
  bits = (__math_high(x) & 0x7fffffUL) | 0x800000UL;
  if ((bits & ((1UL << shift) - 1)) != 0)
    return 0;
  return (bits >> shift) & 1 ? 2 : 1;
}

/* Is rounded inside a type of bits value bits and a sign?  -2^bits is.  */
int __math_fits(double rounded, int bits)
{
  int power = __math_power_double(rounded);

  if (power != bits)
    return power < bits;
  return __math_negative(rounded) &&
	 ((__math_high(rounded) & 0xfffffUL) | __math_low(rounded)) == 0;
}

int __math_fitsf(float rounded, int bits)
{
  int power = __math_power_float(rounded);

  if (power != bits)
    return power < bits;
  return __math_negative(rounded) && (__math_high(rounded) & 0x7fffffUL) == 0;
}
