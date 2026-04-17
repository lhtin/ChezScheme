/* limits.h -- Implementation limits for bare-metal RV64G */

#ifndef _BAREMETAL_LIMITS_H
#define _BAREMETAL_LIMITS_H

/* Use GCC's built-in limits first */
#include_next <limits.h>

/* POSIX limits not provided by GCC */
#ifndef PATH_MAX
#define PATH_MAX        4096
#endif

#ifndef NAME_MAX
#define NAME_MAX        255
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX       LONG_MAX
#endif

#ifndef PIPE_BUF
#define PIPE_BUF        4096
#endif

#ifndef OPEN_MAX
#define OPEN_MAX        256
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   64
#endif

#ifndef LINE_MAX
#define LINE_MAX        2048
#endif

#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX PATH_MAX
#endif

#ifndef IOV_MAX
#define IOV_MAX         1024
#endif

#ifndef NGROUPS_MAX
#define NGROUPS_MAX     65536
#endif

#ifndef ARG_MAX
#define ARG_MAX         131072
#endif

#endif /* _BAREMETAL_LIMITS_H */
