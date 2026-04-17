/* spawn.h -- Process spawn stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SPAWN_H
#define _BAREMETAL_SPAWN_H

#include <sys/types.h>
#include <signal.h>

typedef struct { int __data; } posix_spawnattr_t;
typedef struct { int __data; } posix_spawn_file_actions_t;

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]);
int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[]);

#endif /* _BAREMETAL_SPAWN_H */
