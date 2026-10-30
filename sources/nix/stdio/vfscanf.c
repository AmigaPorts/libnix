/* narrow */
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include "stdio.h"
#include "ami_printf_math.h"

#undef WIDE
#define VSCANF_NAME vfscanf
#include "__scanf_impl.h"
