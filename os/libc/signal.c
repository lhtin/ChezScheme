/* signal.c -- ChezSchemeOS freestanding libc: signal stubs */

#include <stddef.h>
#include <stdint.h>

typedef unsigned long sigset_t;
typedef void (*sighandler_t)(int);
typedef int pid_t;

#define _NSIG 64

struct sigaction {
    union {
        sighandler_t sa_handler;
        void (*sa_sigaction)(int, void *, void *);
    };
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

static sighandler_t signal_handlers[_NSIG];

int sigaction(int signum, const struct sigaction *act,
              struct sigaction *oldact) {
    if (signum < 0 || signum >= _NSIG) return -1;
    if (oldact) {
        oldact->sa_handler = signal_handlers[signum];
        oldact->sa_mask = 0;
        oldact->sa_flags = 0;
    }
    if (act) {
        signal_handlers[signum] = act->sa_handler;
    }
    return 0;
}

sighandler_t signal(int signum, sighandler_t handler) {
    if (signum < 0 || signum >= _NSIG) return (sighandler_t)-1;
    sighandler_t old = signal_handlers[signum];
    signal_handlers[signum] = handler;
    return old;
}

int raise(int sig) {
    (void)sig;
    return 0;
}

int kill(pid_t pid, int sig) {
    (void)pid; (void)sig;
    return 0;
}

int sigemptyset(sigset_t *set) {
    if (set) *set = 0;
    return 0;
}

int sigfillset(sigset_t *set) {
    if (set) *set = ~(sigset_t)0;
    return 0;
}

int sigaddset(sigset_t *set, int signum) {
    if (!set || signum < 1 || signum >= _NSIG) return -1;
    *set |= (1UL << signum);
    return 0;
}

int sigdelset(sigset_t *set, int signum) {
    if (!set || signum < 1 || signum >= _NSIG) return -1;
    *set &= ~(1UL << signum);
    return 0;
}

int sigismember(const sigset_t *set, int signum) {
    if (!set || signum < 1 || signum >= _NSIG) return -1;
    return (*set >> signum) & 1;
}

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    (void)how; (void)set;
    if (oldset) *oldset = 0;
    return 0;
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset) {
    return sigprocmask(how, set, oldset);
}

int sigaltstack(const void *ss, void *old_ss) {
    (void)ss; (void)old_ss;
    return 0;
}

int sigsuspend(const sigset_t *mask) {
    (void)mask;
    return -1;
}

int sigwait(const sigset_t *set, int *sig) {
    (void)set; (void)sig;
    return -1;
}
