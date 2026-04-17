/* iconv.h -- Character set conversion stubs for bare-metal RV64G */

#ifndef _BAREMETAL_ICONV_H
#define _BAREMETAL_ICONV_H

#include <stddef.h>

typedef void *iconv_t;

iconv_t iconv_open(const char *tocode, const char *fromcode);
size_t  iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
              char **outbuf, size_t *outbytesleft);
int     iconv_close(iconv_t cd);

#endif /* _BAREMETAL_ICONV_H */
