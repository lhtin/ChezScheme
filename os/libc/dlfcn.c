/* dlfcn.c -- ChezSchemeOS freestanding libc: dynamic linking stubs */

#include <stddef.h>

void *dlopen(const char *filename, int flags) {
    (void)filename; (void)flags;
    return NULL;
}

void *dlsym(void *handle, const char *symbol) {
    (void)handle; (void)symbol;
    return NULL;
}

int dlclose(void *handle) {
    (void)handle;
    return -1;
}

char *dlerror(void) {
    return "dynamic linking not supported on bare-metal";
}
