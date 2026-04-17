/* math.h -- Math functions for bare-metal RV64G */

#ifndef _BAREMETAL_MATH_H
#define _BAREMETAL_MATH_H

/* Special values -- use GCC builtins */
#define HUGE_VAL    __builtin_huge_val()
#define HUGE_VALF   __builtin_huge_valf()
#define HUGE_VALL   __builtin_huge_vall()
#define INFINITY    __builtin_inff()
#define NAN         __builtin_nanf("")

/* Classification macros */
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define fpclassify(x) __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, (x))
#define isnan(x)      __builtin_isnan(x)
#define isinf(x)      __builtin_isinf(x)
#define isfinite(x)   __builtin_isfinite(x)
#define isnormal(x)   __builtin_isnormal(x)
#define signbit(x)    __builtin_signbit(x)

/* Math constants */
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

/* Trigonometric functions */
double  sin(double x);
double  cos(double x);
double  tan(double x);
double  asin(double x);
double  acos(double x);
double  atan(double x);
double  atan2(double y, double x);

float   sinf(float x);
float   cosf(float x);
float   tanf(float x);
float   asinf(float x);
float   acosf(float x);
float   atanf(float x);
float   atan2f(float y, float x);

/* Hyperbolic functions */
double  sinh(double x);
double  cosh(double x);
double  tanh(double x);
double  asinh(double x);
double  acosh(double x);
double  atanh(double x);

float   sinhf(float x);
float   coshf(float x);
float   tanhf(float x);

/* Exponential and logarithmic functions */
double  exp(double x);
double  exp2(double x);
double  expm1(double x);
double  log(double x);
double  log2(double x);
double  log10(double x);
double  log1p(double x);

float   expf(float x);
float   logf(float x);
float   log2f(float x);
float   log10f(float x);

/* Power functions */
double  pow(double x, double y);
double  sqrt(double x);
double  cbrt(double x);
double  hypot(double x, double y);

float   powf(float x, float y);
float   sqrtf(float x);
float   cbrtf(float x);
float   hypotf(float x, float y);

/* Nearest integer functions */
double  ceil(double x);
double  floor(double x);
double  round(double x);
double  trunc(double x);
double  rint(double x);
double  nearbyint(double x);
long    lround(double x);
long long llround(double x);
long    lrint(double x);

float   ceilf(float x);
float   floorf(float x);
float   roundf(float x);
float   truncf(float x);

/* Remainder functions */
double  fmod(double x, double y);
double  remainder(double x, double y);
double  remquo(double x, double y, int *quo);

float   fmodf(float x, float y);

/* Manipulation functions */
double  ldexp(double x, int exp);
double  frexp(double x, int *exp);
double  modf(double x, double *iptr);
double  scalbn(double x, int n);
double  scalbln(double x, long n);
int     ilogb(double x);
double  logb(double x);
double  nextafter(double x, double y);

float   ldexpf(float x, int exp);
float   frexpf(float x, int *exp);
float   modff(float x, float *iptr);

/* Absolute value and sign */
double  fabs(double x);
double  copysign(double x, double y);
double  fmax(double x, double y);
double  fmin(double x, double y);
double  fdim(double x, double y);

float   fabsf(float x);
float   copysignf(float x, float y);
float   fmaxf(float x, float y);
float   fminf(float x, float y);

/* Error and gamma functions */
double  erf(double x);
double  erfc(double x);
double  tgamma(double x);
double  lgamma(double x);

/* FP multiply-add */
double  fma(double x, double y, double z);
float   fmaf(float x, float y, float z);

#endif /* _BAREMETAL_MATH_H */
