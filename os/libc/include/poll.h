/* poll.h -- Poll stubs for bare-metal RV64G */

#ifndef _BAREMETAL_POLL_H
#define _BAREMETAL_POLL_H

/* Poll event flags */
#define POLLIN      0x0001
#define POLLPRI     0x0002
#define POLLOUT     0x0004
#define POLLERR     0x0008
#define POLLHUP     0x0010
#define POLLNVAL    0x0020
#define POLLRDNORM  0x0040
#define POLLRDBAND  0x0080
#define POLLWRNORM  0x0100
#define POLLWRBAND  0x0200

typedef unsigned long nfds_t;

struct pollfd {
    int     fd;
    short   events;
    short   revents;
};

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
int ppoll(struct pollfd *fds, nfds_t nfds,
          const void *tmo_p, const void *sigmask);

#endif /* _BAREMETAL_POLL_H */
