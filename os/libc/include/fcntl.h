/* fcntl.h -- File control for bare-metal RV64G */

#ifndef _BAREMETAL_FCNTL_H
#define _BAREMETAL_FCNTL_H

#include <sys/types.h>

/* Open flags */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_ACCMODE   0x0003
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_NOCTTY    0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_NONBLOCK  0x0800
#define O_SYNC      0x1000
#define O_BINARY    0       /* No binary/text distinction on bare-metal */
#define O_CLOEXEC   0x80000

/* fcntl commands */
#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4
#define F_GETLK     5
#define F_SETLK     6
#define F_SETLKW    7
#define F_DUPFD_CLOEXEC 1030

/* fd flags */
#define FD_CLOEXEC  1

/* flock operation constants */
#define LOCK_SH     1   /* Shared lock */
#define LOCK_EX     2   /* Exclusive lock */
#define LOCK_NB     4   /* Don't block */
#define LOCK_UN     8   /* Unlock */

struct flock {
    short   l_type;
    short   l_whence;
    off_t   l_start;
    off_t   l_len;
    pid_t   l_pid;
};

int open(const char *pathname, int flags, ...);
int creat(const char *pathname, mode_t mode);
int fcntl(int fd, int cmd, ...);
int flock(int fd, int operation);

/* AT_ constants for *at() functions */
#define AT_FDCWD    (-100)
#define AT_SYMLINK_NOFOLLOW 0x100

#endif /* _BAREMETAL_FCNTL_H */
