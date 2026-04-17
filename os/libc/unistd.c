/* unistd.c -- ChezSchemeOS freestanding libc: POSIX stubs */

#include <stddef.h>
#include <stdint.h>

/* opt* variables for getopt */
char *optarg = NULL;
int optind = 1, opterr = 1, optopt = '?';

extern int errno;
extern void *malloc(size_t);
extern char *strcpy(char *, const char *);
extern size_t strlen(const char *);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
#define ENOENT 2
#define EBADF  9
#define ENOSYS 38

typedef long ssize_t;
typedef long off_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

/* QEMU virt timer: 10 MHz */
#define TIMER_FREQ 10000000UL
static inline uint64_t rdtime_val(void) {
    uint64_t val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}

pid_t getpid(void)   { return 1; }
pid_t getppid(void)  { return 0; }
uid_t getuid(void)   { return 0; }
uid_t geteuid(void)  { return 0; }
gid_t getgid(void)   { return 0; }
gid_t getegid(void)  { return 0; }

char *getcwd(char *buf, size_t size) {
    if (buf && size >= 2) {
        buf[0] = '/';
        buf[1] = '\0';
        return buf;
    }
    errno = ENOENT;
    return NULL;
}

int chdir(const char *path) {
    (void)path;
    errno = ENOENT;
    return -1;
}

int access(const char *pathname, int mode) {
    (void)pathname; (void)mode;
    errno = ENOENT;
    return -1;
}

int ftruncate(int fd, off_t length) {
    (void)fd; (void)length;
    errno = ENOSYS;
    return -1;
}

int dup(int oldfd) {
    (void)oldfd;
    errno = ENOSYS;
    return -1;
}

int dup2(int oldfd, int newfd) {
    (void)oldfd; (void)newfd;
    errno = ENOSYS;
    return -1;
}

int pipe(int pipefd[2]) {
    (void)pipefd;
    errno = ENOSYS;
    return -1;
}

int unlink(const char *pathname) {
    (void)pathname;
    errno = ENOENT;
    return -1;
}

int rmdir(const char *pathname) {
    (void)pathname;
    errno = ENOENT;
    return -1;
}

int isatty(int fd) {
    return (fd >= 0 && fd <= 2) ? 1 : 0;
}

#define _SC_PAGESIZE     30
#define _SC_CLK_TCK      2
#define _SC_NPROCESSORS_ONLN 84

long sysconf(int name) {
    switch (name) {
    case _SC_PAGESIZE: return 4096;
    case _SC_CLK_TCK: return 100;
    case _SC_NPROCESSORS_ONLN: return 1;
    default: return -1;
    }
}

long pathconf(const char *path, int name) {
    (void)path; (void)name;
    return -1;
}

long fpathconf(int fd, int name) {
    (void)fd; (void)name;
    return -1;
}

pid_t fork(void) {
    errno = ENOSYS;
    return -1;
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    (void)filename; (void)argv; (void)envp;
    errno = ENOSYS;
    return -1;
}

int execvp(const char *file, char *const argv[]) {
    (void)file; (void)argv;
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    (void)pathname; (void)buf; (void)bufsiz;
    errno = ENOENT;
    return -1;
}

int symlink(const char *target, const char *linkpath) {
    (void)target; (void)linkpath;
    errno = ENOSYS;
    return -1;
}

int link(const char *oldpath, const char *newpath) {
    (void)oldpath; (void)newpath;
    errno = ENOSYS;
    return -1;
}

int truncate(const char *path, off_t length) {
    (void)path; (void)length;
    errno = ENOSYS;
    return -1;
}

void _exit(int status);

/* Alarm -- no-op */
unsigned int alarm(unsigned int seconds) {
    (void)seconds;
    return 0;
}

/* Pause */
int pause(void) {
    __asm__ volatile("wfi");
    return -1;
}

/* getpagesize */
int getpagesize(void) {
    return 4096;
}

/* gethostname */
int gethostname(char *name, size_t len) {
    const char *host = "schemeos";
    size_t hlen = strlen(host);
    if (hlen >= len) hlen = len - 1;
    memcpy(name, host, hlen);
    name[hlen] = '\0';
    return 0;
}

/* Needed by some libs */
int setuid(uid_t uid) { (void)uid; return 0; }
int setgid(gid_t gid) { (void)gid; return 0; }
int seteuid(uid_t euid) { (void)euid; return 0; }
int setegid(gid_t egid) { (void)egid; return 0; }

/* realpath -- return the path as-is (no filesystem to resolve) */
char *realpath(const char *path, char *resolved_path) {
    if (!path) { errno = ENOENT; return NULL; }
    if (!resolved_path) {
        size_t len = strlen(path) + 1;
        resolved_path = (char *)malloc(len);
        if (!resolved_path) return NULL;
    }
    strcpy(resolved_path, path);
    return resolved_path;
}

int execl(const char *path, const char *arg, ...) {
    (void)path; (void)arg;
    errno = ENOSYS;
    return -1;
}

int setenv(const char *name, const char *value, int overwrite) {
    (void)name; (void)value; (void)overwrite;
    return 0;
}

int unsetenv(const char *name) {
    (void)name;
    return 0;
}

/* pwd stubs */
struct passwd {
    char *pw_name;
    char *pw_passwd;
    unsigned int pw_uid;
    unsigned int pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};

static struct passwd dummy_pw = {
    "root", "*", 0, 0, "root", "/", "/bin/sh"
};

struct passwd *getpwuid(unsigned int uid) {
    (void)uid;
    return &dummy_pw;
}

struct passwd *getpwnam(const char *name) {
    (void)name;
    return &dummy_pw;
}

/* waitpid stub */
pid_t waitpid(pid_t pid, int *wstatus, int options) {
    (void)pid; (void)options;
    if (wstatus) *wstatus = 0;
    errno = ENOSYS;
    return -1;
}

pid_t wait(int *wstatus) {
    return waitpid(-1, wstatus, 0);
}
