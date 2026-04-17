/* arpa/inet.h -- Internet address conversion stubs for bare-metal RV64G */

#ifndef _BAREMETAL_ARPA_INET_H
#define _BAREMETAL_ARPA_INET_H

#include <netinet/in.h>

const char *inet_ntop(int af, const void *src, char *dst, unsigned int size);
int         inet_pton(int af, const char *src, void *dst);
in_addr_t   inet_addr(const char *cp);
char       *inet_ntoa(struct in_addr in);

#endif /* _BAREMETAL_ARPA_INET_H */
