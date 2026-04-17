/* sys/utsname.h -- System identification stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_UTSNAME_H
#define _BAREMETAL_SYS_UTSNAME_H

#define _UTSNAME_LENGTH 65

struct utsname {
    char sysname[_UTSNAME_LENGTH];
    char nodename[_UTSNAME_LENGTH];
    char release[_UTSNAME_LENGTH];
    char version[_UTSNAME_LENGTH];
    char machine[_UTSNAME_LENGTH];
};

int uname(struct utsname *name);

#endif /* _BAREMETAL_SYS_UTSNAME_H */
