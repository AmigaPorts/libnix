#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define errno (*__errno)

extern int * __errno;

#include <sys/errno.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ERRNO_H */
