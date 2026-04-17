/* dirent.c -- ChezSchemeOS freestanding libc: directory stubs */

#include <stddef.h>

extern int errno;
#define ENOENT 2

typedef struct DIR DIR;

struct dirent {
    unsigned long d_ino;
    unsigned char d_type;
    char d_name[256];
};

DIR *opendir(const char *name) {
    (void)name;
    errno = ENOENT;
    return NULL;
}

struct dirent *readdir(DIR *dirp) {
    (void)dirp;
    return NULL;
}

int closedir(DIR *dirp) {
    (void)dirp;
    return 0;
}
