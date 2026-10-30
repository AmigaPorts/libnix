#include <math.h>

// Float versions (wrap the double implementations to satisfy the float linker symbols)
float jnf(int n, float x)  { return (float)jn(n, (double)x); }
float j0f(float x)         { return (float)j0((double)x); }
float j1f(float x)         { return (float)j1((double)x); }
