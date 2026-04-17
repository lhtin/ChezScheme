/* stdlib.c -- ChezSchemeOS freestanding libc: standard library functions */

#include <stddef.h>
#include <stdint.h>

extern void uart_puts(const char *s);
extern void uart_putc(char c);
extern void *memcpy(void *, const void *, size_t);
extern size_t strlen(const char *s);

/* abort and exit */

void abort(void) {
    uart_puts("\nabort() called\n");
    while (1) { __asm__ volatile("wfi"); }
    __builtin_unreachable();
}

void exit(int status) {
    uart_puts("\nexit(");
    if (status < 0) { uart_putc('-'); status = -status; }
    if (status >= 100) uart_putc('0' + status / 100);
    if (status >= 10) uart_putc('0' + (status / 10) % 10);
    uart_putc('0' + status % 10);
    uart_puts(")\n");
    while (1) { __asm__ volatile("wfi"); }
    __builtin_unreachable();
}

void _exit(int status) { exit(status); }
void _Exit(int status) { exit(status); }

/* String-to-integer conversions */

static int _isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long result = 0;
    int neg = 0;

    while (_isspace(*s)) s++;

    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    /* Detect base */
    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X') { base = 16; s++; }
            else { base = 8; }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return neg ? -result : result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long result = 0;

    while (_isspace(*s)) s++;

    if (*s == '+') s++;
    else if (*s == '-') s++; /* technically allowed, wraps around */

    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X') { base = 16; s++; }
            else { base = 8; }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return result;
}

long long strtoll(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long long result = 0;
    int neg = 0;

    while (_isspace(*s)) s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X') { base = 16; s++; }
            else { base = 8; }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return neg ? -result : result;
}

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long long result = 0;

    while (_isspace(*s)) s++;
    if (*s == '+') s++;

    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X') { base = 16; s++; }
            else { base = 8; }
        } else {
            base = 10;
        }
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return result;
}

double strtod(const char *nptr, char **endptr) {
    const char *s = nptr;
    double result = 0.0;
    int neg = 0;

    while (_isspace(*s)) s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    /* Integer part */
    while (*s >= '0' && *s <= '9') {
        result = result * 10.0 + (*s - '0');
        s++;
    }

    /* Fractional part */
    if (*s == '.') {
        s++;
        double place = 0.1;
        while (*s >= '0' && *s <= '9') {
            result += (*s - '0') * place;
            place *= 0.1;
            s++;
        }
    }

    /* Exponent */
    if (*s == 'e' || *s == 'E') {
        s++;
        int eneg = 0;
        int exp = 0;
        if (*s == '-') { eneg = 1; s++; }
        else if (*s == '+') s++;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        double factor = 1.0;
        while (exp-- > 0) factor *= 10.0;
        if (eneg) result /= factor;
        else result *= factor;
    }

    if (endptr) *endptr = (char *)s;
    return neg ? -result : result;
}

float strtof(const char *nptr, char **endptr) {
    return (float)strtod(nptr, endptr);
}

long double strtold(const char *nptr, char **endptr) {
    return (long double)strtod(nptr, endptr);
}

int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }
long long atoll(const char *s) { return strtoll(s, NULL, 10); }
double atof(const char *s) { return strtod(s, NULL); }

int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }

/* qsort -- iterative quicksort */
static void swap_bytes(char *a, char *b, size_t size) {
    while (size--) {
        char tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    if (nmemb <= 1) return;

    /* Simple insertion sort for small arrays */
    if (nmemb <= 16) {
        char *arr = (char *)base;
        for (size_t i = 1; i < nmemb; i++) {
            size_t j = i;
            while (j > 0 && compar(arr + j * size, arr + (j - 1) * size) < 0) {
                swap_bytes(arr + j * size, arr + (j - 1) * size, size);
                j--;
            }
        }
        return;
    }

    /* Quicksort with stack-based iteration */
    struct { size_t lo, hi; } stack[64];
    int top = 0;
    stack[top].lo = 0;
    stack[top].hi = nmemb - 1;
    top++;

    char *arr = (char *)base;
    while (top > 0) {
        top--;
        size_t lo = stack[top].lo;
        size_t hi = stack[top].hi;

        if (lo >= hi) continue;

        /* Median-of-three pivot */
        size_t mid = lo + (hi - lo) / 2;
        if (compar(arr + mid * size, arr + lo * size) < 0)
            swap_bytes(arr + mid * size, arr + lo * size, size);
        if (compar(arr + hi * size, arr + lo * size) < 0)
            swap_bytes(arr + hi * size, arr + lo * size, size);
        if (compar(arr + mid * size, arr + hi * size) < 0)
            swap_bytes(arr + mid * size, arr + hi * size, size);

        /* Pivot is at hi */
        size_t i = lo;
        size_t j = hi - 1;
        while (1) {
            while (i <= j && compar(arr + i * size, arr + hi * size) < 0) i++;
            while (j > i && compar(arr + j * size, arr + hi * size) > 0) j--;
            if (i >= j) break;
            swap_bytes(arr + i * size, arr + j * size, size);
            i++; j--;
        }
        swap_bytes(arr + i * size, arr + hi * size, size);

        if (top < 62) {
            if (i > lo + 1) { stack[top].lo = lo; stack[top].hi = i - 1; top++; }
            if (i + 1 < hi) { stack[top].lo = i + 1; stack[top].hi = hi; top++; }
        }
    }
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
    const char *arr = (const char *)base;
    size_t lo = 0, hi = nmemb;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int cmp = compar(key, arr + mid * size);
        if (cmp < 0) hi = mid;
        else if (cmp > 0) lo = mid + 1;
        else return (void *)(arr + mid * size);
    }
    return NULL;
}

/* getenv -- always NULL on bare-metal */
char *getenv(const char *name) {
    (void)name;
    return NULL;
}

/* Simple LCG PRNG */
static unsigned long _rand_state = 1;

int rand(void) {
    _rand_state = _rand_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (int)((_rand_state >> 33) & 0x7FFFFFFF);
}

void srand(unsigned int seed) {
    _rand_state = seed;
}

/* system -- not supported */
int system(const char *command) {
    (void)command;
    return -1;
}

/* atexit -- stub */
typedef void (*atexit_func)(void);
static atexit_func _atexit_funcs[32];
static int _atexit_count = 0;

int atexit(void (*func)(void)) {
    if (_atexit_count >= 32) return -1;
    _atexit_funcs[_atexit_count++] = func;
    return 0;
}

/* div, ldiv, lldiv */
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t div(int numer, int denom) {
    div_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

ldiv_t ldiv(long numer, long denom) {
    ldiv_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

lldiv_t lldiv(long long numer, long long denom) {
    lldiv_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

/* mkstemp -- stub */
int mkstemp(char *tmpl) {
    (void)tmpl;
    return -1;
}
