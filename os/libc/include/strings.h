/* strings.h -- Legacy string functions for bare-metal RV64G */

#ifndef _BAREMETAL_STRINGS_H
#define _BAREMETAL_STRINGS_H

#include <stddef.h>

int    bcmp(const void *s1, const void *s2, size_t n);
void   bcopy(const void *src, void *dest, size_t n);
void   bzero(void *s, size_t n);
int    strcasecmp(const char *s1, const char *s2);
int    strncasecmp(const char *s1, const char *s2, size_t n);
int    ffs(int i);

#endif /* _BAREMETAL_STRINGS_H */
