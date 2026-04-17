/* string.c -- ChezSchemeOS freestanding libc: string functions */

#include <stddef.h>

extern void *malloc(size_t);

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = s1, *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = s;
    while (n--) {
        if (*p == (unsigned char)c) return (void *)p;
        p++;
    }
    return NULL;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    while (n-- && (*d = *src++)) d++;
    *d = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    size_t nlen = strlen(needle);
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0)
            return (char *)haystack;
        haystack++;
    }
    return NULL;
}

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

char *strndup(const char *s, size_t n) {
    size_t len = strlen(s);
    if (len > n) len = n;
    char *d = malloc(len + 1);
    if (d) {
        memcpy(d, s, len);
        d[len] = '\0';
    }
    return d;
}

static const char *errno_strings[] = {
    [0] = "Success",
    [1] = "Operation not permitted",
    [2] = "No such file or directory",
    [3] = "No such process",
    [4] = "Interrupted system call",
    [5] = "I/O error",
    [9] = "Bad file descriptor",
    [11] = "Resource temporarily unavailable",
    [12] = "Out of memory",
    [13] = "Permission denied",
    [14] = "Bad address",
    [17] = "File exists",
    [20] = "Not a directory",
    [21] = "Is a directory",
    [22] = "Invalid argument",
    [23] = "Too many open files",
    [24] = "Too many open files in system",
    [28] = "No space left on device",
    [29] = "Illegal seek",
    [34] = "Numerical result out of range",
    [36] = "File name too long",
    [38] = "Function not implemented",
};
#define ERRNO_STRINGS_COUNT (sizeof(errno_strings)/sizeof(errno_strings[0]))

char *strerror(int errnum) {
    if (errnum >= 0 && (size_t)errnum < ERRNO_STRINGS_COUNT && errno_strings[errnum])
        return (char *)errno_strings[errnum];
    return (char *)"Unknown error";
}

char *strtok_r(char *str, const char *delim, char **saveptr);

static char *strtok_save;

char *strtok(char *str, const char *delim) {
    return strtok_r(str, delim, &strtok_save);
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    if (str == NULL) str = *saveptr;
    if (str == NULL) return NULL;

    /* Skip leading delimiters */
    while (*str && strchr(delim, *str)) str++;
    if (*str == '\0') { *saveptr = NULL; return NULL; }

    char *token = str;
    while (*str && !strchr(delim, *str)) str++;
    if (*str) {
        *str = '\0';
        *saveptr = str + 1;
    } else {
        *saveptr = NULL;
    }
    return token;
}

size_t strspn(const char *s, const char *accept) {
    size_t count = 0;
    while (*s && strchr(accept, *s)) { s++; count++; }
    return count;
}

size_t strcspn(const char *s, const char *reject) {
    size_t count = 0;
    while (*s && !strchr(reject, *s)) { s++; count++; }
    return count;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        if (strchr(accept, *s)) return (char *)s;
        s++;
    }
    return NULL;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        int c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    int c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
    int c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
    return c1 - c2;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && *s2) {
        int c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        int c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return c1 - c2;
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    int c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
    int c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
    return c1 - c2;
}
