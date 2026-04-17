/* ctype.c -- ChezSchemeOS freestanding libc: character classification */

int isalpha(int c)  { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
int isdigit(int c)  { return c >= '0' && c <= '9'; }
int isalnum(int c)  { return isalpha(c) || isdigit(c); }
int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int isspace(int c)  { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
int isupper(int c)  { return c >= 'A' && c <= 'Z'; }
int islower(int c)  { return c >= 'a' && c <= 'z'; }
int isprint(int c)  { return c >= 0x20 && c <= 0x7e; }
int isgraph(int c)  { return c > 0x20 && c <= 0x7e; }
int iscntrl(int c)  { return (c >= 0 && c < 0x20) || c == 0x7f; }
int ispunct(int c)  { return isprint(c) && !isalnum(c) && c != ' '; }
int isascii(int c)  { return (unsigned)c <= 127; }
int isblank(int c)  { return c == ' ' || c == '\t'; }

int toupper(int c)  { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
int tolower(int c)  { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

/* Wide character stubs */
typedef unsigned int wint_t;
typedef int wchar_t_int;

int towlower(wint_t wc) { return (wc >= 'A' && wc <= 'Z') ? wc + 32 : (int)wc; }
int towupper(wint_t wc) { return (wc >= 'a' && wc <= 'z') ? wc - 32 : (int)wc; }
int iswspace(wint_t wc) { return isspace((int)wc); }
int iswdigit(wint_t wc) { return isdigit((int)wc); }
int iswupper(wint_t wc) { return isupper((int)wc); }
int iswlower(wint_t wc) { return islower((int)wc); }
int iswprint(wint_t wc) { return isprint((int)wc); }
int iswalpha(wint_t wc) { return isalpha((int)wc); }
int iswalnum(wint_t wc) { return isalnum((int)wc); }
int iswpunct(wint_t wc) { return ispunct((int)wc); }
int iswcntrl(wint_t wc) { return iscntrl((int)wc); }
int iswxdigit(wint_t wc) { return isxdigit((int)wc); }
int iswgraph(wint_t wc) { return isgraph((int)wc); }
int iswblank(wint_t wc) { return isblank((int)wc); }
