/* printf.c -- ChezSchemeOS freestanding libc: printf family
 *
 * Core engine: vsnprintf. All other printf variants are wrappers.
 * Output to UART via uart_putc for stdout/stderr.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declarations -- we avoid including full headers to prevent circular deps */
extern void uart_putc(char c);

/* Internal output context */
typedef struct {
    char *buf;
    size_t pos;
    size_t size;
} printf_ctx_t;

static void ctx_putc(printf_ctx_t *ctx, char c) {
    if (ctx->buf && ctx->pos < ctx->size - 1) {
        ctx->buf[ctx->pos] = c;
    }
    ctx->pos++;
}

/* Print an unsigned integer in given base */
static void print_uint(printf_ctx_t *ctx, unsigned long long val, int base,
                       int uppercase, int width, int zero_pad, int left_align,
                       int precision) {
    char digits[64];
    const char *hexchars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    if (val == 0) {
        if (precision != 0)
            digits[i++] = '0';
    } else {
        while (val) {
            digits[i++] = hexchars[val % base];
            val /= base;
        }
    }

    /* Apply precision (minimum digits) */
    while (i < precision) digits[i++] = '0';

    int numlen = i;
    int pad = (width > numlen) ? width - numlen : 0;
    char padchar = zero_pad ? '0' : ' ';

    if (!left_align) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, padchar);
    }
    for (int j = i - 1; j >= 0; j--) {
        ctx_putc(ctx, digits[j]);
    }
    if (left_align) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
    }
}

/* Print a signed integer */
static void print_int(printf_ctx_t *ctx, long long val, int base,
                      int width, int zero_pad, int left_align,
                      int plus_flag, int space_flag, int precision) {
    char prefix = 0;

    if (val < 0) {
        val = -val;
        prefix = '-';
    } else if (plus_flag) {
        prefix = '+';
    } else if (space_flag) {
        prefix = ' ';
    }

    char digits[64];
    int i = 0;
    unsigned long long uval = (unsigned long long)val;

    if (uval == 0) {
        if (precision != 0)
            digits[i++] = '0';
    } else {
        const char *hexchars = "0123456789abcdef";
        while (uval) {
            digits[i++] = hexchars[uval % base];
            uval /= base;
        }
    }

    while (i < precision) digits[i++] = '0';

    int numlen = i + (prefix ? 1 : 0);
    int pad = (width > numlen) ? width - numlen : 0;
    char padchar = zero_pad ? '0' : ' ';

    if (!left_align) {
        if (zero_pad && prefix) ctx_putc(ctx, prefix);
        for (int j = 0; j < pad; j++) ctx_putc(ctx, padchar);
        if (!zero_pad && prefix) ctx_putc(ctx, prefix);
    } else {
        if (prefix) ctx_putc(ctx, prefix);
    }

    for (int j = i - 1; j >= 0; j--) {
        ctx_putc(ctx, digits[j]);
    }

    if (left_align) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
    }
}

/* Print a double -- basic implementation */
static void print_double(printf_ctx_t *ctx, double val, int width,
                         int precision, int left_align, int zero_pad,
                         int plus_flag, int space_flag, char spec) {
    /* Handle special values */
    if (val != val) { /* NaN */
        const char *s = "nan";
        int pad = (width > 3) ? width - 3 : 0;
        if (!left_align) for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
        while (*s) ctx_putc(ctx, *s++);
        if (left_align) for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
        return;
    }

    int negative = 0;
    if (val < 0) { negative = 1; val = -val; }

    /* Check infinity */
    if (val > 1e308) {
        char prefix = negative ? '-' : (plus_flag ? '+' : (space_flag ? ' ' : 0));
        int slen = 3 + (prefix ? 1 : 0);
        int pad = (width > slen) ? width - slen : 0;
        if (!left_align) for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
        if (prefix) ctx_putc(ctx, prefix);
        ctx_putc(ctx, 'i'); ctx_putc(ctx, 'n'); ctx_putc(ctx, 'f');
        if (left_align) for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
        return;
    }

    if (precision < 0) precision = 6;

    /* Handle %e format */
    if (spec == 'e' || spec == 'E') {
        int exponent = 0;
        if (val != 0.0) {
            while (val >= 10.0) { val /= 10.0; exponent++; }
            while (val < 1.0) { val *= 10.0; exponent--; }
        }
        /* Print in format: d.ddddde+dd */
        char prefix = negative ? '-' : (plus_flag ? '+' : (space_flag ? ' ' : 0));
        if (prefix) ctx_putc(ctx, prefix);

        /* Integer part */
        int ipart = (int)val;
        ctx_putc(ctx, '0' + ipart);
        val -= ipart;

        if (precision > 0) {
            ctx_putc(ctx, '.');
            for (int i = 0; i < precision; i++) {
                val *= 10.0;
                int digit = (int)val;
                if (digit > 9) digit = 9;
                ctx_putc(ctx, '0' + digit);
                val -= digit;
            }
        }
        ctx_putc(ctx, spec); /* 'e' or 'E' */
        ctx_putc(ctx, exponent >= 0 ? '+' : '-');
        if (exponent < 0) exponent = -exponent;
        if (exponent < 10) ctx_putc(ctx, '0');
        if (exponent >= 100) {
            ctx_putc(ctx, '0' + exponent / 100);
            exponent %= 100;
        }
        ctx_putc(ctx, '0' + exponent / 10);
        ctx_putc(ctx, '0' + exponent % 10);
        return;
    }

    /* %g: use %e if exponent < -4 or >= precision, else %f */
    if (spec == 'g' || spec == 'G') {
        double test = val;
        int exp = 0;
        if (test != 0.0) {
            while (test >= 10.0) { test /= 10.0; exp++; }
            while (test < 1.0) { test *= 10.0; exp--; }
        }
        if (exp < -4 || exp >= precision) {
            if (negative) val = -val;
            print_double(ctx, negative ? -val : val, width, precision > 0 ? precision - 1 : 0,
                        left_align, zero_pad, plus_flag, space_flag,
                        spec == 'g' ? 'e' : 'E');
            return;
        }
        /* Fall through to %f */
    }

    /* %f format */
    /* Round */
    double rounding = 0.5;
    for (int i = 0; i < precision; i++) rounding /= 10.0;
    val += rounding;

    unsigned long long ipart = (unsigned long long)val;
    double fpart = val - (double)ipart;

    /* Count total length for padding */
    char prefix = negative ? '-' : (plus_flag ? '+' : (space_flag ? ' ' : 0));

    /* Build integer part */
    char ibuf[32];
    int ilen = 0;
    if (ipart == 0) {
        ibuf[ilen++] = '0';
    } else {
        unsigned long long tmp = ipart;
        while (tmp) { ibuf[ilen++] = '0' + (tmp % 10); tmp /= 10; }
    }

    int total_len = ilen + (prefix ? 1 : 0) + (precision > 0 ? 1 + precision : 0);
    int pad = (width > total_len) ? width - total_len : 0;

    if (!left_align && !zero_pad) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
    }
    if (prefix) ctx_putc(ctx, prefix);
    if (!left_align && zero_pad) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, '0');
    }

    for (int j = ilen - 1; j >= 0; j--) ctx_putc(ctx, ibuf[j]);

    if (precision > 0) {
        ctx_putc(ctx, '.');
        for (int i = 0; i < precision; i++) {
            fpart *= 10.0;
            int digit = (int)fpart;
            if (digit > 9) digit = 9;
            ctx_putc(ctx, '0' + digit);
            fpart -= digit;
        }
    }

    if (left_align) {
        for (int j = 0; j < pad; j++) ctx_putc(ctx, ' ');
    }
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    printf_ctx_t ctx = { buf, 0, size };

    while (*fmt) {
        if (*fmt != '%') {
            ctx_putc(&ctx, *fmt++);
            continue;
        }
        fmt++; /* skip '%' */

        /* Flags */
        int zero_pad = 0, left_align = 0, plus_flag = 0, space_flag = 0, hash_flag = 0;
        while (1) {
            if (*fmt == '0') { zero_pad = 1; fmt++; }
            else if (*fmt == '-') { left_align = 1; fmt++; }
            else if (*fmt == '+') { plus_flag = 1; fmt++; }
            else if (*fmt == ' ') { space_flag = 1; fmt++; }
            else if (*fmt == '#') { hash_flag = 1; fmt++; }
            else break;
        }
        if (left_align) zero_pad = 0;

        /* Width */
        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) { left_align = 1; width = -width; }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        /* Precision */
        int precision = -1;
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            if (*fmt == '*') {
                precision = va_arg(ap, int);
                if (precision < 0) precision = -1;
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        /* Length modifier */
        int length = 0; /* 0=int, 1=long, 2=long long, 3=size_t */
        if (*fmt == 'l') {
            fmt++;
            length = 1;
            if (*fmt == 'l') { fmt++; length = 2; }
        } else if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') fmt++; /* hh */
        } else if (*fmt == 'z') {
            fmt++;
            length = 3; /* size_t */
        } else if (*fmt == 'j') {
            fmt++;
            length = 2; /* intmax_t = long long */
        } else if (*fmt == 't') {
            fmt++;
            length = 1; /* ptrdiff_t = long on 64-bit */
        }

        /* Conversion */
        char spec = *fmt++;
        (void)hash_flag;

        switch (spec) {
        case 'd': case 'i': {
            long long val;
            if (length == 2) val = va_arg(ap, long long);
            else if (length == 1 || length == 3) val = va_arg(ap, long);
            else val = va_arg(ap, int);
            int p = (precision >= 0) ? precision : 1;
            print_int(&ctx, val, 10, width, (precision < 0) ? zero_pad : 0,
                     left_align, plus_flag, space_flag, p);
            break;
        }
        case 'u': {
            unsigned long long val;
            if (length == 2) val = va_arg(ap, unsigned long long);
            else if (length == 1 || length == 3) val = va_arg(ap, unsigned long);
            else val = va_arg(ap, unsigned int);
            int p = (precision >= 0) ? precision : 1;
            print_uint(&ctx, val, 10, 0, width, (precision < 0) ? zero_pad : 0,
                      left_align, p);
            break;
        }
        case 'x': case 'X': {
            unsigned long long val;
            if (length == 2) val = va_arg(ap, unsigned long long);
            else if (length == 1 || length == 3) val = va_arg(ap, unsigned long);
            else val = va_arg(ap, unsigned int);
            if (hash_flag && val != 0) {
                ctx_putc(&ctx, '0');
                ctx_putc(&ctx, spec); /* 'x' or 'X' */
                if (width > 2) width -= 2;
                else width = 0;
            }
            int p = (precision >= 0) ? precision : 1;
            print_uint(&ctx, val, 16, (spec == 'X'), width,
                      (precision < 0) ? zero_pad : 0, left_align, p);
            break;
        }
        case 'o': {
            unsigned long long val;
            if (length == 2) val = va_arg(ap, unsigned long long);
            else if (length == 1 || length == 3) val = va_arg(ap, unsigned long);
            else val = va_arg(ap, unsigned int);
            int p = (precision >= 0) ? precision : 1;
            print_uint(&ctx, val, 8, 0, width, (precision < 0) ? zero_pad : 0,
                      left_align, p);
            break;
        }
        case 'p': {
            void *ptr = va_arg(ap, void *);
            ctx_putc(&ctx, '0');
            ctx_putc(&ctx, 'x');
            print_uint(&ctx, (unsigned long long)(uintptr_t)ptr, 16, 0,
                      0, 0, 0, 1);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            size_t slen = 0;
            const char *p = s;
            while (*p) { slen++; p++; }
            if (precision >= 0 && (size_t)precision < slen) slen = precision;
            int pad = (width > (int)slen) ? width - (int)slen : 0;
            if (!left_align) {
                for (int j = 0; j < pad; j++) ctx_putc(&ctx, ' ');
            }
            for (size_t j = 0; j < slen; j++) ctx_putc(&ctx, s[j]);
            if (left_align) {
                for (int j = 0; j < pad; j++) ctx_putc(&ctx, ' ');
            }
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            int pad = (width > 1) ? width - 1 : 0;
            if (!left_align) {
                for (int j = 0; j < pad; j++) ctx_putc(&ctx, ' ');
            }
            ctx_putc(&ctx, c);
            if (left_align) {
                for (int j = 0; j < pad; j++) ctx_putc(&ctx, ' ');
            }
            break;
        }
        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G': {
            double val = va_arg(ap, double);
            print_double(&ctx, val, width, precision, left_align, zero_pad,
                        plus_flag, space_flag, spec);
            break;
        }
        case 'n': {
            if (length == 2) {
                long long *p = va_arg(ap, long long *);
                *p = (long long)ctx.pos;
            } else if (length == 1) {
                long *p = va_arg(ap, long *);
                *p = (long)ctx.pos;
            } else {
                int *p = va_arg(ap, int *);
                *p = (int)ctx.pos;
            }
            break;
        }
        case '%':
            ctx_putc(&ctx, '%');
            break;
        default:
            ctx_putc(&ctx, '%');
            ctx_putc(&ctx, spec);
            break;
        }
    }

    /* Null terminate */
    if (buf) {
        if (ctx.pos < size) buf[ctx.pos] = '\0';
        else if (size > 0) buf[size - 1] = '\0';
    }

    return (int)ctx.pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, (size_t)-1, fmt, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
    return vsnprintf(buf, (size_t)-1, fmt, ap);
}

/* FILE-based output: uses uart_putc for stdout/stderr */
struct _FILE;
typedef struct _FILE FILE;
extern FILE *stdout;
extern FILE *stderr;
extern int fileno(FILE *);

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    int fd = fileno(stream);
    if (fd == 1 || fd == 2) {
        for (int i = 0; i < len && i < (int)sizeof(buf) - 1; i++) {
            if (buf[i] == '\n') uart_putc('\r');
            uart_putc(buf[i]);
        }
    }
    return len;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stream, fmt, ap);
    va_end(ap);
    return ret;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}
