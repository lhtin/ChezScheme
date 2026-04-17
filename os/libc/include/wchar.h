/* wchar.h -- Wide character support for bare-metal RV64G */

#ifndef _BAREMETAL_WCHAR_H
#define _BAREMETAL_WCHAR_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

#ifndef _WINT_T
#define _WINT_T
typedef unsigned int wint_t;
#endif

/* Multibyte conversion state */
typedef struct {
    int __count;
    union {
        unsigned int __wch;
        char         __wchb[4];
    } __value;
} mbstate_t;

/* Multibyte/wide string conversion */
size_t  mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t  wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
size_t  mbrlen(const char *s, size_t n, mbstate_t *ps);
size_t  mbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *ps);
size_t  wcsrtombs(char *dest, const wchar_t **src, size_t len, mbstate_t *ps);
size_t  mbstowcs(wchar_t *dest, const char *src, size_t n);
size_t  wcstombs(char *dest, const wchar_t *src, size_t n);
int     mbsinit(const mbstate_t *ps);
int     mbtowc(wchar_t *pwc, const char *s, size_t n);
int     wctomb(char *s, wchar_t wc);
wint_t  btowc(int c);
int     wctob(wint_t c);
int     mblen(const char *s, size_t n);

/* Wide string functions */
size_t  wcslen(const wchar_t *s);
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wcscat(wchar_t *dest, const wchar_t *src);
wchar_t *wcsncat(wchar_t *dest, const wchar_t *src, size_t n);
int     wcscmp(const wchar_t *s1, const wchar_t *s2);
int     wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wcschr(const wchar_t *wcs, wchar_t wc);
wchar_t *wcsrchr(const wchar_t *wcs, wchar_t wc);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
size_t  wcsspn(const wchar_t *wcs, const wchar_t *accept);
size_t  wcscspn(const wchar_t *wcs, const wchar_t *reject);
int     wcscoll(const wchar_t *ws1, const wchar_t *ws2);
size_t  wcsxfrm(wchar_t *dest, const wchar_t *src, size_t n);

/* Wide memory functions */
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *wcs, wchar_t wc, size_t n);
int     wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);

/* Wide char I/O */
wint_t  fgetwc(FILE *stream);
wint_t  fputwc(wchar_t wc, FILE *stream);
wchar_t *fgetws(wchar_t *ws, int n, FILE *stream);
int     fputws(const wchar_t *ws, FILE *stream);
wint_t  getwc(FILE *stream);
wint_t  putwc(wchar_t wc, FILE *stream);
wint_t  getwchar(void);
wint_t  putwchar(wchar_t wc);
wint_t  ungetwc(wint_t wc, FILE *stream);

/* Wide char number conversion */
long            wcstol(const wchar_t *nptr, wchar_t **endptr, int base);
unsigned long   wcstoul(const wchar_t *nptr, wchar_t **endptr, int base);
long long       wcstoll(const wchar_t *nptr, wchar_t **endptr, int base);
unsigned long long wcstoull(const wchar_t *nptr, wchar_t **endptr, int base);
double          wcstod(const wchar_t *nptr, wchar_t **endptr);
float           wcstof(const wchar_t *nptr, wchar_t **endptr);

/* Wide char formatted I/O */
int     swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *fmt, ...);
int     vswprintf(wchar_t *wcs, size_t maxlen, const wchar_t *fmt, va_list ap);
int     fwprintf(FILE *stream, const wchar_t *fmt, ...);
int     wprintf(const wchar_t *fmt, ...);
int     swscanf(const wchar_t *ws, const wchar_t *fmt, ...);

/* Time */
size_t  wcsftime(wchar_t *wcs, size_t maxsize, const wchar_t *format, const struct tm *timeptr);

/* Duplicate */
wchar_t *wcsdup(const wchar_t *s);

#endif /* _BAREMETAL_WCHAR_H */
