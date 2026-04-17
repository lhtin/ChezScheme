/* locale.c -- ChezSchemeOS freestanding libc: locale stubs */

#include <stddef.h>

typedef void *locale_t;

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
};

static struct lconv _c_locale = {
    .decimal_point = ".",
    .thousands_sep = "",
    .grouping = "",
    .int_curr_symbol = "",
    .currency_symbol = "",
    .mon_decimal_point = "",
    .mon_thousands_sep = "",
    .mon_grouping = "",
    .positive_sign = "",
    .negative_sign = "",
    .int_frac_digits = 127,
    .frac_digits = 127,
    .p_cs_precedes = 127,
    .p_sep_by_space = 127,
    .n_cs_precedes = 127,
    .n_sep_by_space = 127,
    .p_sign_posn = 127,
    .n_sign_posn = 127,
};

char *setlocale(int category, const char *locale) {
    (void)category; (void)locale;
    return "C";
}

struct lconv *localeconv(void) {
    return &_c_locale;
}

locale_t uselocale(locale_t newloc) {
    (void)newloc;
    return (locale_t)0;
}

locale_t newlocale(int category_mask, const char *locale, locale_t base) {
    (void)category_mask; (void)locale; (void)base;
    return (locale_t)1; /* Return non-null dummy */
}

void freelocale(locale_t locale) {
    (void)locale;
}

char *nl_langinfo(int item) {
    (void)item;
    return "";
}

char *nl_langinfo_l(int item, locale_t locale) {
    (void)item; (void)locale;
    return "";
}

/* mbrtowc_l, etc. used by Chez on some platforms */
typedef int mbstate_t_local;

size_t mbrtowc_l(int *pwc, const char *s, size_t n, mbstate_t_local *ps, locale_t loc) {
    (void)ps; (void)loc;
    if (!s) return 0;
    if (n == 0) return (size_t)-2;
    if (pwc) *pwc = (unsigned char)*s;
    return *s ? 1 : 0;
}
