/* math.c -- ChezSchemeOS freestanding libc math functions for RV64G
 *
 * RV64G has hardware double-precision FP (D extension).
 * We use GCC builtins where possible, and simple polynomial
 * approximations for transcendental functions.
 */

#include <math.h>

/* --- Basic operations using HW FP or builtins --- */

double fabs(double x) { return __builtin_fabs(x); }
float fabsf(float x) { return __builtin_fabsf(x); }

double copysign(double x, double y) { return __builtin_copysign(x, y); }
float copysignf(float x, float y) { return __builtin_copysignf(x, y); }

double fmax(double x, double y) { return __builtin_fmax(x, y); }
double fmin(double x, double y) { return __builtin_fmin(x, y); }

double sqrt(double x) { return __builtin_sqrt(x); }
float sqrtf(float x) { return __builtin_sqrtf(x); }

double ceil(double x) { return __builtin_ceil(x); }
double floor(double x) { return __builtin_floor(x); }
double round(double x) { return __builtin_round(x); }
double trunc(double x) { return __builtin_trunc(x); }
double rint(double x) { return __builtin_rint(x); }
float ceilf(float x) { return __builtin_ceilf(x); }
float floorf(float x) { return __builtin_floorf(x); }
float roundf(float x) { return __builtin_roundf(x); }
float truncf(float x) { return __builtin_truncf(x); }

long lround(double x) { return (long)__builtin_round(x); }
long long llround(double x) { return (long long)__builtin_round(x); }
long lrint(double x) { return (long)__builtin_rint(x); }

double fmod(double x, double y) {
    if (y == 0.0) return NAN;
    double q = trunc(x / y);
    return x - q * y;
}
float fmodf(float x, float y) { return (float)fmod(x, y); }

double remainder(double x, double y) {
    if (y == 0.0) return NAN;
    double q = round(x / y);
    return x - q * y;
}

/* --- IEEE 754 bit manipulation --- */

typedef union { double d; unsigned long long u; } du_t;
typedef union { float f; unsigned int u; } fu_t;

double ldexp(double x, int exp) {
    /* x * 2^exp */
    while (exp > 0) { x *= 2.0; exp--; }
    while (exp < 0) { x *= 0.5; exp++; }
    return x;
}

double frexp(double x, int *exp) {
    if (x == 0.0) { *exp = 0; return 0.0; }
    du_t u;
    u.d = x;
    int e = (int)((u.u >> 52) & 0x7FF) - 1022;
    *exp = e;
    u.u = (u.u & 0x800FFFFFFFFFFFFFULL) | 0x3FE0000000000000ULL;
    return u.d;
}

double modf(double x, double *iptr) {
    double i = trunc(x);
    *iptr = i;
    return x - i;
}

double scalbn(double x, int n) { return ldexp(x, n); }

int ilogb(double x) {
    if (x == 0.0) return -2147483647;
    du_t u;
    u.d = fabs(x);
    return (int)((u.u >> 52) & 0x7FF) - 1023;
}

double logb(double x) { return (double)ilogb(x); }

/* --- Exponential and logarithmic --- */

/* Constants */
static const double LN2     = 0.6931471805599453;
static const double LOG2E   = 1.4426950408889634;
static const double LN10    = 2.302585092994046;

/* exp(x) using range reduction: exp(x) = 2^k * exp(r) where r = x - k*ln2, |r| < ln2/2 */
double exp(double x) {
    if (__builtin_isnan(x)) return x;
    if (x > 709.0) return HUGE_VAL;
    if (x < -745.0) return 0.0;

    double k = floor(x * LOG2E + 0.5);
    double r = x - k * LN2;

    /* Pade approximation for exp(r) - 1, |r| < 0.35 */
    double r2 = r * r;
    double r3 = r2 * r;
    double p = r + r2 * 0.5 + r3 / 6.0 + r2 * r2 / 24.0 + r2 * r3 / 120.0
             + r3 * r3 / 720.0 + r2 * r2 * r3 / 5040.0;
    double result = 1.0 + p;

    return ldexp(result, (int)k);
}
float expf(float x) { return (float)exp(x); }

double exp2(double x) { return exp(x * LN2); }

double expm1(double x) {
    if (fabs(x) < 1e-5) return x + 0.5 * x * x;
    return exp(x) - 1.0;
}

/* log(x) using decomposition: x = m * 2^e, log(x) = e*ln2 + log(m), 1 <= m < 2 */
double log(double x) {
    if (x < 0.0) return NAN;
    if (x == 0.0) return -HUGE_VAL;
    if (__builtin_isnan(x) || __builtin_isinf(x)) return x;

    int e;
    double m = frexp(x, &e);
    /* m is in [0.5, 1.0), adjust to [1.0, 2.0) */
    m *= 2.0;
    e -= 1;

    /* log(m) for m in [1, 2] using polynomial on t = (m-1)/(m+1) */
    double t = (m - 1.0) / (m + 1.0);
    double t2 = t * t;
    /* log(m) = 2*t * (1 + t^2/3 + t^4/5 + t^6/7 + ...) */
    double sum = 1.0 + t2 * (1.0/3.0 + t2 * (1.0/5.0 + t2 * (1.0/7.0
                + t2 * (1.0/9.0 + t2 * (1.0/11.0 + t2 / 13.0)))));
    double logm = 2.0 * t * sum;

    return (double)e * LN2 + logm;
}
float logf(float x) { return (float)log(x); }

double log2(double x) { return log(x) * LOG2E; }
double log10(double x) { return log(x) / LN10; }
float log2f(float x) { return (float)log2(x); }
float log10f(float x) { return (float)log10(x); }

double log1p(double x) {
    if (fabs(x) < 1e-8) return x - 0.5 * x * x;
    return log(1.0 + x);
}

double pow(double x, double y) {
    if (y == 0.0) return 1.0;
    if (x == 0.0) return 0.0;
    if (x == 1.0) return 1.0;
    if (y == 1.0) return x;
    if (y == 2.0) return x * x;
    if (y == -1.0) return 1.0 / x;

    /* Integer exponent fast path */
    if (y == floor(y) && fabs(y) < 63) {
        int n = (int)y;
        if (n < 0) { x = 1.0 / x; n = -n; }
        double result = 1.0;
        while (n > 0) {
            if (n & 1) result *= x;
            x *= x;
            n >>= 1;
        }
        return result;
    }

    if (x < 0.0) {
        /* Negative base with non-integer exponent = NaN */
        return NAN;
    }
    return exp(y * log(x));
}
float powf(float x, float y) { return (float)pow(x, y); }

double cbrt(double x) {
    if (x == 0.0) return 0.0;
    double s = (x > 0.0) ? 1.0 : -1.0;
    double a = fabs(x);
    /* Newton's method: y = y - (y^3 - a) / (3 * y^2) */
    double y = exp(log(a) / 3.0);
    y = y - (y * y * y - a) / (3.0 * y * y);
    y = y - (y * y * y - a) / (3.0 * y * y);
    return s * y;
}

/* --- Trigonometric --- */

static const double PI     = 3.14159265358979323846;
static const double PI_2   = 1.57079632679489661923;
static const double TWO_PI = 6.28318530717958647692;

/* Reduce x to [-pi, pi] */
static double reduce_angle(double x) {
    x = fmod(x, TWO_PI);
    if (x > PI) x -= TWO_PI;
    if (x < -PI) x += TWO_PI;
    return x;
}

/* sin(x) for x in [-pi/4, pi/4] using minimax polynomial */
static double sin_kern(double x) {
    double x2 = x * x;
    return x * (1.0 + x2 * (-1.0/6.0 + x2 * (1.0/120.0 + x2 * (-1.0/5040.0
         + x2 * (1.0/362880.0 + x2 * (-1.0/39916800.0 + x2 / 6227020800.0))))));
}

/* cos(x) for x in [-pi/4, pi/4] using minimax polynomial */
static double cos_kern(double x) {
    double x2 = x * x;
    return 1.0 + x2 * (-0.5 + x2 * (1.0/24.0 + x2 * (-1.0/720.0
         + x2 * (1.0/40320.0 + x2 * (-1.0/3628800.0 + x2 / 479001600.0)))));
}

double sin(double x) {
    if (__builtin_isnan(x) || __builtin_isinf(x)) return NAN;
    x = reduce_angle(x);
    if (x > PI_2) return cos_kern(x - PI_2);
    if (x < -PI_2) return -cos_kern(x + PI_2);
    return sin_kern(x);
}

double cos(double x) {
    if (__builtin_isnan(x) || __builtin_isinf(x)) return NAN;
    x = reduce_angle(x);
    if (x > PI_2) return -sin_kern(x - PI_2);
    if (x < -PI_2) return -sin_kern(x + PI_2);
    return cos_kern(x);
}

double tan(double x) {
    double c = cos(x);
    if (c == 0.0) return copysign(HUGE_VAL, sin(x));
    return sin(x) / c;
}

float sinf(float x) { return (float)sin(x); }
float cosf(float x) { return (float)cos(x); }
float tanf(float x) { return (float)tan(x); }

/* --- Inverse trigonometric --- */

/* atan(x) for |x| <= 1 */
static double atan_kern(double x) {
    double x2 = x * x;
    return x * (1.0 + x2 * (-1.0/3.0 + x2 * (1.0/5.0 + x2 * (-1.0/7.0
         + x2 * (1.0/9.0 + x2 * (-1.0/11.0 + x2 * (1.0/13.0
         + x2 * (-1.0/15.0 + x2 / 17.0))))))));
}

double atan(double x) {
    if (__builtin_isnan(x)) return x;
    if (x > 1.0) return PI_2 - atan_kern(1.0 / x);
    if (x < -1.0) return -PI_2 - atan_kern(1.0 / x);
    return atan_kern(x);
}

double atan2(double y, double x) {
    if (__builtin_isnan(x) || __builtin_isnan(y)) return NAN;
    if (x > 0.0) return atan(y / x);
    if (x < 0.0) {
        if (y >= 0.0) return atan(y / x) + PI;
        return atan(y / x) - PI;
    }
    /* x == 0 */
    if (y > 0.0) return PI_2;
    if (y < 0.0) return -PI_2;
    return 0.0;
}

double asin(double x) {
    if (fabs(x) > 1.0) return NAN;
    if (fabs(x) == 1.0) return copysign(PI_2, x);
    return atan2(x, sqrt(1.0 - x * x));
}

double acos(double x) {
    if (fabs(x) > 1.0) return NAN;
    return PI_2 - asin(x);
}

float atanf(float x) { return (float)atan(x); }
float atan2f(float y, float x) { return (float)atan2(y, x); }
float asinf(float x) { return (float)asin(x); }
float acosf(float x) { return (float)acos(x); }

/* --- Hyperbolic --- */

double sinh(double x) {
    if (fabs(x) < 1e-5) return x;
    double e = exp(x);
    return (e - 1.0/e) * 0.5;
}

double cosh(double x) {
    double e = exp(x);
    return (e + 1.0/e) * 0.5;
}

double tanh(double x) {
    if (x > 20.0) return 1.0;
    if (x < -20.0) return -1.0;
    double e2 = exp(2.0 * x);
    return (e2 - 1.0) / (e2 + 1.0);
}

double asinh(double x) {
    return log(x + sqrt(x * x + 1.0));
}

double acosh(double x) {
    if (x < 1.0) return NAN;
    return log(x + sqrt(x * x - 1.0));
}

double atanh(double x) {
    if (fabs(x) >= 1.0) return copysign(HUGE_VAL, x);
    return 0.5 * log((1.0 + x) / (1.0 - x));
}

float sinhf(float x) { return (float)sinh(x); }
float coshf(float x) { return (float)cosh(x); }
float tanhf(float x) { return (float)tanh(x); }

/* --- Misc --- */

double hypot(double x, double y) {
    x = fabs(x);
    y = fabs(y);
    if (x < y) { double t = x; x = y; y = t; }
    if (x == 0.0) return 0.0;
    double r = y / x;
    return x * sqrt(1.0 + r * r);
}

float hypotf(float x, float y) { return (float)hypot(x, y); }

double erf(double x) {
    /* Abramowitz and Stegun approximation */
    double a = fabs(x);
    double t = 1.0 / (1.0 + 0.3275911 * a);
    double y = 1.0 - (((((1.061405429 * t - 1.453152027) * t)
                + 1.421413741) * t - 0.284496736) * t + 0.254829592) * t
                * exp(-a * a);
    return copysign(y, x);
}

double erfc(double x) { return 1.0 - erf(x); }

double tgamma(double x) {
    /* Stirling's approximation for large x */
    if (x < 0.5) return PI / (sin(PI * x) * tgamma(1.0 - x));
    x -= 1.0;
    /* Lanczos approximation with g=7 */
    static const double c[] = {
        0.99999999999980993,
        676.5203681218851,
        -1259.1392167224028,
        771.32342877765313,
        -176.61502916214059,
        12.507343278686905,
        -0.13857109526572012,
        9.9843695780195716e-6,
        1.5056327351493116e-7
    };
    double sum = c[0];
    for (int i = 1; i < 9; i++) sum += c[i] / (x + (double)i);
    double t = x + 7.5;
    return sqrt(TWO_PI) * pow(t, x + 0.5) * exp(-t) * sum;
}

double lgamma(double x) { return log(fabs(tgamma(x))); }

/* Floating-point classification helpers */
int __fpclassifyf(float x) {
    fu_t u;
    u.f = x;
    unsigned int e = (u.u >> 23) & 0xFF;
    unsigned int m = u.u & 0x7FFFFF;
    if (e == 0xFF) return m ? FP_NAN : FP_INFINITE;
    if (e == 0) return m ? FP_SUBNORMAL : FP_ZERO;
    return FP_NORMAL;
}

int __fpclassifyd(double x) {
    du_t u;
    u.d = x;
    unsigned long long e = (u.u >> 52) & 0x7FF;
    unsigned long long m = u.u & 0x000FFFFFFFFFFFFFULL;
    if (e == 0x7FF) return m ? FP_NAN : FP_INFINITE;
    if (e == 0) return m ? FP_SUBNORMAL : FP_ZERO;
    return FP_NORMAL;
}

int __signbitf(float x) {
    fu_t u;
    u.f = x;
    return (u.u >> 31) & 1;
}

int __signbitd(double x) {
    du_t u;
    u.d = x;
    return (int)(u.u >> 63);
}

double nan(const char *s) {
    (void)s;
    return NAN;
}

double nextafter(double x, double y) {
    if (__builtin_isnan(x) || __builtin_isnan(y)) return NAN;
    if (x == y) return y;
    du_t u;
    u.d = x;
    if (x == 0.0) {
        u.u = 1;
        return copysign(u.d, y);
    }
    if ((x > 0.0) == (y > x))
        u.u++;
    else
        u.u--;
    return u.d;
}
