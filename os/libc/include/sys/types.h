/* sys/types.h -- Basic system types for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_TYPES_H
#define _BAREMETAL_SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef long            ssize_t;
typedef long            off_t;
typedef int             pid_t;
typedef unsigned int    uid_t;
typedef unsigned int    gid_t;
typedef unsigned int    mode_t;
typedef unsigned long   dev_t;
typedef unsigned long   ino_t;
typedef unsigned long   nlink_t;
typedef long            blksize_t;
typedef long            blkcnt_t;
typedef long            time_t;
typedef long            clock_t;
typedef long            suseconds_t;
typedef int             clockid_t;
typedef unsigned long   fsblkcnt_t;
typedef unsigned long   fsfilcnt_t;
typedef long            key_t;
typedef int             id_t;
typedef unsigned int    useconds_t;

#endif /* _BAREMETAL_SYS_TYPES_H */
