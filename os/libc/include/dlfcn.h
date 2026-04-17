/* dlfcn.h -- Dynamic loading stubs for bare-metal RV64G */

#ifndef _BAREMETAL_DLFCN_H
#define _BAREMETAL_DLFCN_H

#define RTLD_LAZY     0x00001
#define RTLD_NOW      0x00002
#define RTLD_GLOBAL   0x00100
#define RTLD_LOCAL    0x00000
#define RTLD_NOLOAD   0x00004
#define RTLD_NODELETE 0x01000
#define RTLD_DEFAULT  ((void *)0)
#define RTLD_NEXT     ((void *)-1)

void   *dlopen(const char *filename, int flags);
int     dlclose(void *handle);
void   *dlsym(void *handle, const char *symbol);
char   *dlerror(void);

/* GNU extension */
typedef struct {
    const char *dli_fname;
    void       *dli_fbase;
    const char *dli_sname;
    void       *dli_saddr;
} Dl_info;

int dladdr(const void *addr, Dl_info *info);

#endif /* _BAREMETAL_DLFCN_H */
