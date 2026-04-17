/* sys/mman.h -- Memory mapping stubs for bare-metal RV64G
 * We use USE_MALLOC so these are largely unused, but headers
 * may still be included. */

#ifndef _BAREMETAL_SYS_MMAN_H
#define _BAREMETAL_SYS_MMAN_H

#include <sys/types.h>

/* Protection flags */
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

/* Map flags */
#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_32BIT       0x40
#define MAP_NORESERVE   0x4000

#define MAP_FAILED      ((void *)-1)

/* msync flags */
#define MS_ASYNC    1
#define MS_SYNC     4
#define MS_INVALIDATE 2

/* madvise flags */
#define MADV_NORMAL     0
#define MADV_RANDOM     1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED   3
#define MADV_DONTNEED   4

void   *mmap(void *addr, size_t length, int prot, int flags,
             int fd, off_t offset);
int     munmap(void *addr, size_t length);
int     mprotect(void *addr, size_t len, int prot);
int     msync(void *addr, size_t length, int flags);
int     madvise(void *addr, size_t length, int advice);
int     mlock(const void *addr, size_t len);
int     munlock(const void *addr, size_t len);
int     mlockall(int flags);
int     munlockall(void);

#endif /* _BAREMETAL_SYS_MMAN_H */
