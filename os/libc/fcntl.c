/* fcntl.c -- ChezSchemeOS freestanding libc: fcntl stubs */

#include <stdarg.h>

extern int errno;
#define ENOSYS 38
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2

int fcntl(int fd, int cmd, ...) {
    va_list ap;
    va_start(ap, cmd);
    (void)fd;

    switch (cmd) {
    case F_GETFL:
        va_end(ap);
        return 0;  /* Return 0 flags (blocking mode) */
    case F_SETFL: {
        int flags = va_arg(ap, int);
        (void)flags;
        va_end(ap);
        return 0;  /* Pretend success */
    }
    case F_GETFD:
        va_end(ap);
        return 0;
    case F_SETFD:
        va_end(ap);
        return 0;
    case F_DUPFD:
        va_end(ap);
        errno = ENOSYS;
        return -1;
    default:
        va_end(ap);
        return 0;
    }
}

int flock(int fd, int operation) {
    (void)fd; (void)operation;
    return 0; /* no-op, pretend success */
}

int lockf(int fd, int cmd, long len) {
    (void)fd; (void)cmd; (void)len;
    return 0;
}
