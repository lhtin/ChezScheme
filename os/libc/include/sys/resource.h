/* sys/resource.h -- Resource usage stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_RESOURCE_H
#define _BAREMETAL_SYS_RESOURCE_H

#include <sys/time.h>
#include <sys/types.h>

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)
#define RUSAGE_THREAD   1

struct rusage {
    struct timeval ru_utime;    /* user CPU time used */
    struct timeval ru_stime;    /* system CPU time used */
    long   ru_maxrss;           /* maximum resident set size */
    long   ru_ixrss;
    long   ru_idrss;
    long   ru_isrss;
    long   ru_minflt;
    long   ru_majflt;
    long   ru_nswap;
    long   ru_inblock;
    long   ru_oublock;
    long   ru_msgsnd;
    long   ru_msgrcv;
    long   ru_nsignals;
    long   ru_nvcsw;
    long   ru_nivcsw;
};

/* Resource limits */
#define RLIMIT_CPU      0
#define RLIMIT_FSIZE    1
#define RLIMIT_DATA     2
#define RLIMIT_STACK    3
#define RLIMIT_CORE     4
#define RLIMIT_NOFILE   7
#define RLIMIT_AS       9

#define RLIM_INFINITY   (~0UL)

typedef unsigned long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

int     getrusage(int who, struct rusage *usage);
int     getrlimit(int resource, struct rlimit *rlim);
int     setrlimit(int resource, const struct rlimit *rlim);

/* Priority */
#define PRIO_PROCESS    0
#define PRIO_PGRP       1
#define PRIO_USER       2

int     getpriority(int which, id_t who);
int     setpriority(int which, id_t who, int prio);

#endif /* _BAREMETAL_SYS_RESOURCE_H */
