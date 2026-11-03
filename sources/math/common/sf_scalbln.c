/* s_scalbnf.c -- float version of s_scalbn.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "fdlibm.h"
#include "math_errno.h"

#ifdef __STDC__
static const float
#else
static float
#endif
two25   =  3.355443200e+07,	/* 0x4c000000 */
twom25  =  2.9802322388e-08,	/* 0x33000000 */
huge   = 1.0e+30,
tiny   = 1.0e-30;

#ifdef __STDC__
	float scalblnf (float x, long int n)
#else
	float scalblnf (x,n)
	float x; long int n;
#endif
{
	__int32_t k,ix;
	GET_FLOAT_WORD(ix,x);
        k = (ix&0x7f800000)>>23;		/* extract exponent */
        if (k==0) {				/* 0 or subnormal x */
            if ((ix&0x7fffffff)==0) return x; /* +-0 */
	    x *= two25;
	    GET_FLOAT_WORD(ix,x);
	    k = ((ix&0x7f800000)>>23) - 25;
	    }
        if (k==0xff) return __MATH_SOFT(x+x, x);		/* NaN or Inf */
        k = k+n;
        if (n> 50000 || k >  0xfe)
	  return __MATH_SOFT(huge*copysignf(huge,x), __math_overflowf(x)); /* overflow  */
	if (n< -50000)
	  return __MATH_SOFT(tiny*copysignf(tiny,x), __math_underflowf(x));	/*underflow*/
        if (k > 0) 				/* normal result */
	    {SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23)); return x;}
        if (k <= -25)
	    return __MATH_SOFT(tiny*copysignf(tiny,x), __math_underflowf(x));	/*underflow*/
        k += 25;				/* subnormal result */
	SET_FLOAT_WORD(x,(ix&0x807fffff)|(k<<23));
        x *= twom25;
	/* a result that rounds to 0 underflows too */
	return __MATH_SOFT(x, x == 0 ? __math_rangef(x) : x);
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double scalbln (double x, long int n)
#else
	double scalbln (x,n)
	double x; long int n;
#endif
{
	return (double) scalblnf((float) x, n);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
