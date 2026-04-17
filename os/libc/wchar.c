/* wchar.c -- ChezSchemeOS freestanding libc: wide character support
 *
 * Simple ASCII-passthrough implementation.
 */

#include <stddef.h>
#include <stdint.h>

typedef int wchar_t_int;  /* wchar_t is built-in in C */
typedef unsigned int wint_t;
typedef struct { int __count; unsigned int __value; } mbstate_t;

#define WEOF ((wint_t)-1)

int mbsinit(const mbstate_t *ps) {
    if (!ps) return 1;
    return ps->__count == 0;
}

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps) {
    (void)ps;
    if (!s) return 0;
    if (n == 0) return (size_t)-2;

    unsigned char c = (unsigned char)*s;
    if (c == 0) {
        if (pwc) *pwc = 0;
        return 0;
    }

    /* Simple UTF-8 decoding */
    if (c < 0x80) {
        if (pwc) *pwc = c;
        return 1;
    } else if (c < 0xC0) {
        /* Invalid start byte */
        return (size_t)-1;
    } else if (c < 0xE0) {
        if (n < 2) return (size_t)-2;
        if (pwc) *pwc = ((c & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
        return 2;
    } else if (c < 0xF0) {
        if (n < 3) return (size_t)-2;
        if (pwc) *pwc = ((c & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6)
                        | ((unsigned char)s[2] & 0x3F);
        return 3;
    } else if (c < 0xF8) {
        if (n < 4) return (size_t)-2;
        if (pwc) *pwc = ((c & 0x07) << 18) | (((unsigned char)s[1] & 0x3F) << 12)
                        | (((unsigned char)s[2] & 0x3F) << 6) | ((unsigned char)s[3] & 0x3F);
        return 4;
    }
    return (size_t)-1;
}

size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps) {
    (void)ps;
    if (!s) return 1;

    unsigned int c = (unsigned int)wc;
    if (c < 0x80) {
        s[0] = (char)c;
        return 1;
    } else if (c < 0x800) {
        s[0] = (char)(0xC0 | (c >> 6));
        s[1] = (char)(0x80 | (c & 0x3F));
        return 2;
    } else if (c < 0x10000) {
        s[0] = (char)(0xE0 | (c >> 12));
        s[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        s[2] = (char)(0x80 | (c & 0x3F));
        return 3;
    } else if (c < 0x110000) {
        s[0] = (char)(0xF0 | (c >> 18));
        s[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        s[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        s[3] = (char)(0x80 | (c & 0x3F));
        return 4;
    }
    return (size_t)-1;
}

size_t mbrlen(const char *s, size_t n, mbstate_t *ps) {
    return mbrtowc(NULL, s, n, ps);
}

size_t mbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *ps) {
    size_t count = 0;
    while (count < len) {
        wchar_t wc;
        size_t ret = mbrtowc(&wc, *src, 4, ps);
        if (ret == 0) break;
        if (ret == (size_t)-1 || ret == (size_t)-2) return (size_t)-1;
        if (dest) dest[count] = wc;
        *src += ret;
        count++;
    }
    return count;
}

size_t wcsrtombs(char *dest, const wchar_t **src, size_t len, mbstate_t *ps) {
    size_t count = 0;
    char buf[4];
    while (count < len) {
        if (**src == 0) break;
        size_t ret = wcrtomb(buf, **src, ps);
        if (ret == (size_t)-1) return (size_t)-1;
        if (count + ret > len) break;
        if (dest) {
            for (size_t i = 0; i < ret; i++) dest[count + i] = buf[i];
        }
        count += ret;
        (*src)++;
    }
    return count;
}

size_t wcslen(const wchar_t *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) dest[i] = src[i];
    return dest;
}

wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n) {
    for (size_t i = 0; i < n; i++) s[i] = c;
    return s;
}

wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n) {
    if (dest < src) {
        for (size_t i = 0; i < n; i++) dest[i] = src[i];
    } else {
        for (size_t i = n; i > 0; i--) dest[i-1] = src[i-1];
    }
    return dest;
}

int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] < s2[i] ? -1 : 1;
    }
    return 0;
}

wint_t btowc(int c) {
    if (c < 0 || c > 127) return WEOF;
    return (wint_t)c;
}

int wctob(wint_t c) {
    if (c > 127) return -1;
    return (int)c;
}

size_t mbstowcs(wchar_t *dest, const char *src, size_t n) {
    const char *p = src;
    return mbsrtowcs(dest, &p, n, NULL);
}

size_t wcstombs(char *dest, const wchar_t *src, size_t n) {
    return wcsrtombs(dest, &src, n, NULL);
}

int wcscmp(const wchar_t *s1, const wchar_t *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}

wchar_t *wcscpy(wchar_t *dest, const wchar_t *src) {
    wchar_t *d = dest;
    while ((*d++ = *src++));
    return dest;
}

wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
    return dest;
}

wchar_t *wcschr(const wchar_t *s, wchar_t c) {
    while (*s) {
        if (*s == c) return (wchar_t *)s;
        s++;
    }
    return c == 0 ? (wchar_t *)s : NULL;
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t c) {
    const wchar_t *last = NULL;
    while (*s) {
        if (*s == c) last = s;
        s++;
    }
    return c == 0 ? (wchar_t *)s : (wchar_t *)last;
}

int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...) {
    (void)s; (void)n; (void)fmt;
    return 0; /* stub */
}

int mblen(const char *s, size_t n) {
    if (!s) return 0;
    return (int)mbrlen(s, n, NULL);
}

int mbtowc(wchar_t *pwc, const char *s, size_t n) {
    if (!s) return 0;
    size_t ret = mbrtowc(pwc, s, n, NULL);
    if (ret == (size_t)-1 || ret == (size_t)-2) return -1;
    return (int)ret;
}

int wctomb(char *s, wchar_t wc) {
    if (!s) return 0;
    size_t ret = wcrtomb(s, wc, NULL);
    if (ret == (size_t)-1) return -1;
    return (int)ret;
}
