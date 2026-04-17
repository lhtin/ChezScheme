/* sys/wait.h -- Process wait stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_WAIT_H
#define _BAREMETAL_SYS_WAIT_H

#include <sys/types.h>

/* Wait option flags */
#define WNOHANG    1
#define WUNTRACED  2
#define WCONTINUED 8

/* Status macros */
#define WIFEXITED(s)    (((s) & 0x7f) == 0)
#define WEXITSTATUS(s)  (((s) & 0xff00) >> 8)
#define WIFSIGNALED(s)  (((s) & 0x7f) > 0 && ((s) & 0x7f) < 0x7f)
#define WTERMSIG(s)     ((s) & 0x7f)
#define WIFSTOPPED(s)   (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)     WEXITSTATUS(s)
#define WIFCONTINUED(s) ((s) == 0xffff)
#define WCOREDUMP(s)    ((s) & 0x80)

pid_t   wait(int *wstatus);
pid_t   waitpid(pid_t pid, int *wstatus, int options);
int     waitid(int idtype, id_t id, void *infop, int options);

#endif /* _BAREMETAL_SYS_WAIT_H */
