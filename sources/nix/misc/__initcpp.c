#include <stddef.h>
#include "stabs.h"

typedef void (*func_ptr) (void);

extern __far func_ptr __CTOR_LIST__[];
extern __far func_ptr __DTOR_LIST__[];

void __initcpp() {
  func_ptr *p0 = __CTOR_LIST__ + 1;
  func_ptr *p;
  for (p = p0; *p; p++);
  while (p > p0)
    (*--p)();
}

/* Static, and advanced before each call: a destructor that calls exit()
   runs the exit list again, which must go on with the next destructor.  */
void __exitcpp() {
  static func_ptr *p = __DTOR_LIST__ + 1;
  while (*p)
    (*p++)();
}

ADD2INIT(__initcpp,-5);
ADD2EXIT(__exitcpp,-5);
