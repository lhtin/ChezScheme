/* sys_stat.c -- ChezSchemeOS freestanding libc: stat/fstat stubs */

#include <stddef.h>
#include <stdint.h>

extern void *memset(void *, int, size_t);
extern int errno;
#define ENOENT 2
#define EBADF  9
#define ENOSYS 38

#define S_IFMT   0170000
#define S_IFREG  0100000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFLNK  0120000

typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned int mode_t;
typedef unsigned long nlink_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long off_t;
typedef long blksize_t;
typedef long blkcnt_t;
typedef long time_t;

struct timespec_stat {
    time_t tv_sec;
    long tv_nsec;
};

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
    struct timespec_stat st_atim;
    struct timespec_stat st_mtim;
    struct timespec_stat st_ctim;
};

int stat(const char *pathname, struct stat *statbuf) {
    (void)pathname;
    (void)statbuf;
    errno = ENOENT;
    return -1;
}

int lstat(const char *pathname, struct stat *statbuf) {
    (void)pathname;
    (void)statbuf;
    errno = ENOENT;
    return -1;
}

int fstat(int fd, struct stat *statbuf) {
    if (fd >= 0 && fd <= 2) {
        if (statbuf) {
            memset(statbuf, 0, sizeof(*statbuf));
            statbuf->st_mode = S_IFCHR | 0666;
            statbuf->st_blksize = 1024;
        }
        return 0;
    }
    errno = EBADF;
    return -1;
}

/* Also provide fstat64 for compatibility */
int fstat64(int fd, struct stat *statbuf) {
    return fstat(fd, statbuf);
}

int stat64(const char *pathname, struct stat *statbuf) {
    return stat(pathname, statbuf);
}

int lstat64(const char *pathname, struct stat *statbuf) {
    return lstat(pathname, statbuf);
}

int mkdir(const char *pathname, mode_t mode) {
    (void)pathname; (void)mode;
    errno = ENOSYS;
    return -1;
}

int chmod(const char *pathname, mode_t mode) {
    (void)pathname; (void)mode;
    errno = ENOENT;
    return -1;
}

int fchmod(int fd, mode_t mode) {
    (void)fd; (void)mode;
    errno = EBADF;
    return -1;
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    (void)pathname; (void)owner; (void)group;
    errno = ENOENT;
    return -1;
}

mode_t umask(mode_t mask) {
    (void)mask;
    return 022;
}
