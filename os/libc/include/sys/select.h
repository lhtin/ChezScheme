/* sys/select.h -- select() stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_SELECT_H
#define _BAREMETAL_SYS_SELECT_H

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#define FD_SETSIZE  1024

typedef struct {
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set;

#define FD_ZERO(set)    do { \
    unsigned long *__p = (unsigned long *)(set); \
    for (int __i = 0; __i < (int)(sizeof(fd_set)/sizeof(unsigned long)); __i++) \
        __p[__i] = 0; \
} while(0)
#define FD_SET(fd, set)   ((set)->fds_bits[(fd) / (8*sizeof(unsigned long))] |= (1UL << ((fd) % (8*sizeof(unsigned long)))))
#define FD_CLR(fd, set)   ((set)->fds_bits[(fd) / (8*sizeof(unsigned long))] &= ~(1UL << ((fd) % (8*sizeof(unsigned long)))))
#define FD_ISSET(fd, set) (((set)->fds_bits[(fd) / (8*sizeof(unsigned long))] >> ((fd) % (8*sizeof(unsigned long)))) & 1)

int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
int pselect(int nfds, fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds, const struct timespec *timeout,
            const sigset_t *sigmask);

#endif /* _BAREMETAL_SYS_SELECT_H */
