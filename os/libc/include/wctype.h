/* wctype.h -- Wide character classification for bare-metal RV64G */

#ifndef _BAREMETAL_WCTYPE_H
#define _BAREMETAL_WCTYPE_H

#ifndef _WINT_T
#define _WINT_T
typedef unsigned int wint_t;
#endif

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

typedef unsigned long wctype_t;
typedef const int *wctrans_t;

/* Wide character classification */
int     iswalpha(wint_t wc);
int     iswdigit(wint_t wc);
int     iswalnum(wint_t wc);
int     iswspace(wint_t wc);
int     iswupper(wint_t wc);
int     iswlower(wint_t wc);
int     iswprint(wint_t wc);
int     iswpunct(wint_t wc);
int     iswcntrl(wint_t wc);
int     iswxdigit(wint_t wc);
int     iswgraph(wint_t wc);
int     iswblank(wint_t wc);

/* Wide character conversion */
wint_t  towupper(wint_t wc);
wint_t  towlower(wint_t wc);
wint_t  towctrans(wint_t wc, wctrans_t desc);

/* Type/trans lookup */
wctype_t    wctype(const char *name);
wctrans_t   wctrans(const char *name);

int     iswctype(wint_t wc, wctype_t desc);

/* Locale-aware variants */
int     iswalpha_l(wint_t wc, void *locale);
int     iswdigit_l(wint_t wc, void *locale);
int     iswalnum_l(wint_t wc, void *locale);
int     iswspace_l(wint_t wc, void *locale);
int     iswupper_l(wint_t wc, void *locale);
int     iswlower_l(wint_t wc, void *locale);
int     iswprint_l(wint_t wc, void *locale);
int     iswpunct_l(wint_t wc, void *locale);
int     iswcntrl_l(wint_t wc, void *locale);
int     iswxdigit_l(wint_t wc, void *locale);
int     iswgraph_l(wint_t wc, void *locale);
int     iswblank_l(wint_t wc, void *locale);
wint_t  towupper_l(wint_t wc, void *locale);
wint_t  towlower_l(wint_t wc, void *locale);
wctype_t    wctype_l(const char *name, void *locale);
wctrans_t   wctrans_l(const char *name, void *locale);
int     iswctype_l(wint_t wc, wctype_t desc, void *locale);
wint_t  towctrans_l(wint_t wc, wctrans_t desc, void *locale);

#endif /* _BAREMETAL_WCTYPE_H */
