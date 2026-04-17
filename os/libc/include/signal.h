/* signal.h -- Signal handling stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SIGNAL_H
#define _BAREMETAL_SIGNAL_H

#include <sys/types.h>

/* Signal numbers */
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      SIGABRT
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1    10
#define SIGSEGV    11
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
#define SIGTERM    15
#define SIGSTKFLT  16
#define SIGCHLD    17
#define SIGCONT    18
#define SIGSTOP    19
#define SIGTSTP    20
#define SIGTTIN    21
#define SIGTTOU    22
#define SIGURG     23
#define SIGXCPU    24
#define SIGXFSZ    25
#define SIGVTALRM  26
#define SIGPROF    27
#define SIGWINCH   28
#define SIGIO      29
#define SIGPWR     30
#define SIGSYS     31
#define _NSIG      65

/* Signal action flags */
#define SA_NOCLDSTOP  1
#define SA_NOCLDWAIT  2
#define SA_SIGINFO    4
#define SA_RESTART    0x10000000
#define SA_NODEFER    0x40000000
#define SA_RESETHAND  0x80000000
#define SA_ONSTACK    0x08000000

/* sigprocmask how */
#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2

/* Signal handler special values */
typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

/* sigset_t -- simple bitmask */
typedef struct {
    unsigned long __val[_NSIG / (8 * sizeof(unsigned long))];
} sigset_t;

/* siginfo_t */
typedef struct {
    int      si_signo;
    int      si_errno;
    int      si_code;
    pid_t    si_pid;
    uid_t    si_uid;
    int      si_status;
    void    *si_addr;
    long     si_band;
    int      si_fd;
} siginfo_t;

/* Signal action codes */
#define SI_USER     0
#define SI_KERNEL   0x80

/* struct sigaction */
struct sigaction {
    union {
        sighandler_t sa_handler;
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } __sigaction_handler;
    sigset_t sa_mask;
    int      sa_flags;
    void   (*sa_restorer)(void);
};

#define sa_handler   __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction

/* Signal functions */
sighandler_t signal(int signum, sighandler_t handler);
int     raise(int sig);
int     kill(pid_t pid, int sig);
int     sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int     sigemptyset(sigset_t *set);
int     sigfillset(sigset_t *set);
int     sigaddset(sigset_t *set, int signum);
int     sigdelset(sigset_t *set, int signum);
int     sigismember(const sigset_t *set, int signum);
int     sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int     sigpending(sigset_t *set);
int     sigsuspend(const sigset_t *mask);
int     sigwait(const sigset_t *set, int *sig);

/* Stack for signal handlers */
typedef struct {
    void   *ss_sp;
    int     ss_flags;
    size_t  ss_size;
} stack_t;

int     sigaltstack(const stack_t *ss, stack_t *old_ss);

/* Minimal ucontext for Chez Scheme's signal handlers */
typedef struct {
    unsigned long __dummy;
} mcontext_t;

typedef struct ucontext_t {
    unsigned long     uc_flags;
    struct ucontext_t *uc_link;
    stack_t           uc_stack;
    mcontext_t        uc_mcontext;
    sigset_t          uc_sigmask;
} ucontext_t;

#endif /* _BAREMETAL_SIGNAL_H */
