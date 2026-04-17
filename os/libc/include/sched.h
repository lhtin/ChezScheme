/* sched.h -- Scheduling stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SCHED_H
#define _BAREMETAL_SCHED_H

#include <sys/types.h>

struct sched_param {
    int sched_priority;
};

#define SCHED_OTHER  0
#define SCHED_FIFO   1
#define SCHED_RR     2

int sched_yield(void);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_getscheduler(pid_t pid);

/* CPU affinity - minimal stubs */
typedef struct {
    unsigned long __bits[16];
} cpu_set_t;

#define CPU_SETSIZE  (sizeof(cpu_set_t) * 8)
#define CPU_ZERO(set)     do { for (int i=0; i<16; i++) (set)->__bits[i]=0; } while(0)
#define CPU_SET(cpu, set) ((set)->__bits[(cpu)/64] |= (1UL << ((cpu)%64)))
#define CPU_CLR(cpu, set) ((set)->__bits[(cpu)/64] &= ~(1UL << ((cpu)%64)))
#define CPU_ISSET(cpu, set) (((set)->__bits[(cpu)/64] >> ((cpu)%64)) & 1)

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);

#endif /* _BAREMETAL_SCHED_H */
