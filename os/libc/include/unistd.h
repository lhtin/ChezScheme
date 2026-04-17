/* unistd.h -- POSIX API stubs for bare-metal RV64G */

#ifndef _BAREMETAL_UNISTD_H
#define _BAREMETAL_UNISTD_H

#include <sys/types.h>
#include <stddef.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* sysconf names */
#define _SC_PAGESIZE        30
#define _SC_PAGE_SIZE       _SC_PAGESIZE
#define _SC_CLK_TCK         2
#define _SC_NPROCESSORS_ONLN 84
#define _SC_OPEN_MAX        4

/* access() mode flags */
#define F_OK    0
#define R_OK    4
#define W_OK    2
#define X_OK    1

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     close(int fd);
off_t   lseek(int fd, off_t offset, int whence);

int     getpid(void);
int     getppid(void);
uid_t   getuid(void);
uid_t   geteuid(void);
gid_t   getgid(void);
gid_t   getegid(void);

char   *getcwd(char *buf, size_t size);
int     chdir(const char *path);
int     fchdir(int fd);
int     access(int fd, int mode);
int     unlink(const char *pathname);
int     rmdir(const char *pathname);
int     link(const char *oldpath, const char *newpath);
int     symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

int     isatty(int fd);
long    sysconf(int name);
unsigned int sleep(unsigned int seconds);
int     usleep(useconds_t usec);

int     dup(int oldfd);
int     dup2(int oldfd, int newfd);
int     pipe(int pipefd[2]);

pid_t   fork(void);
int     execve(const char *pathname, char *const argv[], char *const envp[]);
int     execvp(const char *file, char *const argv[]);
void    _exit(int status) __attribute__((noreturn));

int     ftruncate(int fd, off_t length);
int     truncate(const char *path, off_t length);
int     fsync(int fd);

int     gethostname(char *name, size_t len);

extern char   *optarg;
extern int     optind, opterr, optopt;
int     getopt(int argc, char * const argv[], const char *optstring);

#endif /* _BAREMETAL_UNISTD_H */
