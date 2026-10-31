/* wide */
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <wchar.h>
#include <wctype.h>
#include "stdio.h"

#define WIDE
#define VSCANF_NAME vfwscanf
#include "__scanf_impl.h"
