/* grp.h -- Group database stubs for bare-metal RV64G */

#ifndef _BAREMETAL_GRP_H
#define _BAREMETAL_GRP_H

#include <sys/types.h>

struct group {
    char   *gr_name;
    char   *gr_passwd;
    gid_t   gr_gid;
    char  **gr_mem;
};

struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);

#endif /* _BAREMETAL_GRP_H */
