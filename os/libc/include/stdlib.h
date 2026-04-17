/* stdlib.h -- Standard library for bare-metal RV64G */

#ifndef _BAREMETAL_STDLIB_H
#define _BAREMETAL_STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     2147483647

void   *malloc(size_t size);
void    free(void *ptr);
void   *realloc(void *ptr, size_t size);
void   *calloc(size_t nmemb, size_t size);

void    abort(void) __attribute__((noreturn));
void    exit(int status) __attribute__((noreturn));
void    _Exit(int status) __attribute__((noreturn));
int     atexit(void (*func)(void));

int     atoi(const char *nptr);
long    atol(const char *nptr);
long long atoll(const char *nptr);
double  atof(const char *nptr);

long            strtol(const char *nptr, char **endptr, int base);
unsigned long   strtoul(const char *nptr, char **endptr, int base);
long long       strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double          strtod(const char *nptr, char **endptr);
float           strtof(const char *nptr, char **endptr);
long double     strtold(const char *nptr, char **endptr);

void    qsort(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void   *bsearch(const void *key, const void *base, size_t nmemb,
                size_t size, int (*compar)(const void *, const void *));

int     abs(int j);
long    labs(long j);
long long llabs(long long j);

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

div_t   div(int numer, int denom);
ldiv_t  ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

char   *getenv(const char *name);
int     setenv(const char *name, const char *value, int overwrite);
int     unsetenv(const char *name);
int     putenv(char *string);

int     rand(void);
void    srand(unsigned int seed);
int     rand_r(unsigned int *seedp);

int     mkstemp(char *tmpl);
char   *mktemp(char *tmpl);
char   *realpath(const char *path, char *resolved_path);
int     system(const char *command);

#endif /* _BAREMETAL_STDLIB_H */
