/* iconv.c -- ChezSchemeOS freestanding libc: iconv stubs */

#include <stddef.h>

extern int errno;
#define EINVAL 22

typedef void *iconv_t;

iconv_t iconv_open(const char *tocode, const char *fromcode) {
    (void)tocode; (void)fromcode;
    /* Return a dummy non-error handle to avoid init failures */
    return (iconv_t)1;
}

size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
             char **outbuf, size_t *outbytesleft) {
    (void)cd;
    /* Simple passthrough for ASCII/UTF-8 compatible data */
    if (!inbuf || !*inbuf || !outbuf || !*outbuf) return 0;

    size_t converted = 0;
    while (*inbytesleft > 0 && *outbytesleft > 0) {
        **outbuf = **inbuf;
        (*inbuf)++;
        (*outbuf)++;
        (*inbytesleft)--;
        (*outbytesleft)--;
        converted++;
    }
    return converted;
}

int iconv_close(iconv_t cd) {
    (void)cd;
    return 0;
}
