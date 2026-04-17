/* sys/ioctl.h -- ioctl stubs for bare-metal RV64G */

#ifndef _BAREMETAL_SYS_IOCTL_H
#define _BAREMETAL_SYS_IOCTL_H

/* Terminal window size */
#define TIOCGWINSZ  0x5413
#define TIOCSWINSZ  0x5414
#define FIONREAD    0x541B

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

int ioctl(int fd, unsigned long request, ...);

#endif /* _BAREMETAL_SYS_IOCTL_H */
