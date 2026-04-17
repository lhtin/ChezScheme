/* netdb.h -- Network database stubs for bare-metal RV64G */

#ifndef _BAREMETAL_NETDB_H
#define _BAREMETAL_NETDB_H

#include <sys/socket.h>

struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    socklen_t        ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
};

struct hostent {
    char   *h_name;
    char  **h_aliases;
    int     h_addrtype;
    int     h_length;
    char  **h_addr_list;
};

#define h_addr h_addr_list[0]

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int errcode);
struct hostent *gethostbyname(const char *name);
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags);

#define AI_PASSIVE     0x01
#define AI_CANONNAME   0x02
#define AI_NUMERICHOST 0x04
#define NI_MAXHOST     1025
#define NI_MAXSERV     32

#define EAI_NONAME     (-2)
#define EAI_AGAIN      (-3)
#define EAI_FAIL       (-4)
#define EAI_FAMILY     (-6)
#define EAI_MEMORY     (-10)
#define EAI_SYSTEM     (-11)

#endif /* _BAREMETAL_NETDB_H */
